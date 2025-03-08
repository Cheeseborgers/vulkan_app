#include "application.hpp"

#include <glm/ext.hpp> // TODO: Check is this the best way to include glm for vulkan coords
#include <glm/glm.hpp>

#include "utility/timer.hpp"

#include "logger.hpp"

#include <thread>

namespace {
struct UniformData {
    glm::mat4 WVP;
};
}

Application::Application(WindowSize window_size)
    : p_window{nullptr},
      m_window_size{window_size},
      p_device{VK_NULL_HANDLE},
      p_vk_queue{nullptr},
      m_number_of_images{},
      p_render_pass{VK_NULL_HANDLE},
      p_vertex_shader{VK_NULL_HANDLE},
      p_fragment_shader{VK_NULL_HANDLE},
      p_pipeline{nullptr},
      m_is_iconified{false},
      m_current_frame{0}
{
}

Application::~Application()
{
    APP_LOG_INFO("Cleaning up application");

    vkDeviceWaitIdle(p_device); // Ensure GPU is idle before cleanup

    for (auto fence : m_frame_fences) {
        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(p_device, fence, nullptr);
            fence = VK_NULL_HANDLE;
        }
    }

    m_vk_core.FreeCommandBuffers(static_cast<u32>(m_command_buffers.size()), m_command_buffers.data());

    m_vk_core.DestroyFramebuffers(m_frame_buffers);

    vkDestroyShaderModule(m_vk_core.GetDevice(), p_vertex_shader, nullptr);
    vkDestroyShaderModule(m_vk_core.GetDevice(), p_fragment_shader, nullptr);

    vkDestroyRenderPass(m_vk_core.GetDevice(), p_render_pass, nullptr);

    m_mesh.Destroy(p_device);

    for (auto uniform_buffer : m_uniform_buffers) {
        uniform_buffer.Destroy(p_device);
    }
}

void Application::Init(std::string_view application_title)
{
    APP_LOG_INFO("Initializing");

    p_window = GoudaVK::GLFW::vulkan_init(m_window_size, application_title, true).value_or(nullptr);

    m_vk_core.Init(p_window, application_title, {1, 3, 0, 0}, m_time_settings.vsync_mode);

    p_device = m_vk_core.GetDevice();
    m_number_of_images = m_vk_core.GetNumberOfImages();
    p_vk_queue = m_vk_core.GetQueue();
    p_render_pass = m_vk_core.CreateSimpleRenderPass();
    m_frame_buffers = m_vk_core.CreateFramebuffers(p_render_pass);

    CreateShaders();
    CreateMesh();
    CreateUniformBuffers();
    CreatePipeline();
    CreateCommandBuffers();
    RecordCommandBuffers();
    CreateFences();
    SetupCallbacks();

    m_audio_manager.Initialize();
    m_laser_1.Load("assets/audio/sound_effects/laser1.wav");
    m_laser_2.Load("assets/audio/sound_effects/laser2.wav");

    m_music2.Load("assets/audio/music_tracks/moonlight.wav");
    m_music.Load("assets/audio/music_tracks/track.mp3");
    m_music3.Load("assets/audio/music_tracks/blondie.mp3");

    m_audio_manager.QueueMusic(m_music3, false);
    m_audio_manager.QueueMusic(m_music2, false);
    m_audio_manager.QueueMusic(m_music, false);
    m_audio_manager.ShuffleRemainingTracks();
    m_audio_manager.PlayMusic();
}

void Application::Update(f32 delta_time) {}

void Application::RenderScene(f32 delta_time)
{
    while (!p_vk_queue->IsSwapchainValid()) {
        std::this_thread::yield();
        ENGINE_LOG_INFO("Rendering paused, waiting for valid swapchain");
    }

    vkWaitForFences(p_device, 1, &m_frame_fences[m_current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(p_device, 1, &m_frame_fences[m_current_frame]);

    u32 image_index{p_vk_queue->AcquireNextImage(m_current_frame)};

    UpdateUniformBuffer(image_index, delta_time);

    p_vk_queue->Submit(m_command_buffers[image_index], m_current_frame, m_frame_fences[m_current_frame]);
    p_vk_queue->Present(image_index, m_current_frame);

    m_current_frame = (m_current_frame + 1) % p_vk_queue->GetMaxFramesInFlight();
}

void Application::Execute()
{
    constexpr f64 sleep_duration{0.001}; // Small sleep to reduce CPU usage
    f32 delta_time{0.0f};

    Gouda::FrameTimer frame_timer;
    Gouda::FixedTimer physics_timer(m_time_settings.fixed_timestep);
    Gouda::GameClock game_clock;

    // Only use FPS limiter if V-Sync is disabled
    std::optional<Gouda::FrameRateLimiter> fps_limiter;
    if (m_time_settings.vsync_mode == GoudaVK::VSyncMode::DISABLED) {
        fps_limiter.emplace(m_time_settings.target_fps);
    }

    while (!glfwWindowShouldClose(p_window)) {

        glfwPollEvents();
        m_audio_manager.Update(); // TODO: IS this where we want to update audio? also do we pause audio when minimised?

        frame_timer.Update();
        delta_time = frame_timer.GetDeltaTime();

        // Apply time scaling
        delta_time = game_clock.ApplyTimeScale(delta_time);

        if (m_is_iconified) {
            glfwWaitEventsTimeout(sleep_duration); // Avoid busy waiting
            continue;                              // Skip rendering while minimized
        }

        // Update physics at a fixed timestep
        physics_timer.UpdateAccumulator(delta_time);
        while (physics_timer.ShouldUpdate()) {
            Update(physics_timer.GetFixedTimeStep());
            physics_timer.Advance();
        }

        RenderScene(delta_time); // Render at variable FPS

        // Apply FPS limiter only if V-Sync is off
        if (fps_limiter) {
            fps_limiter->Limit(delta_time);
        }
    }

    vkDeviceWaitIdle(p_device);  // TODO: Use vk_core wait
    glfwDestroyWindow(p_window); // TODO: add cleanup to vk_glfw .hpp / create window wrapper?
    glfwTerminate();
}

// Private -------------------------------------------------------------------------------------------------------------
void Application::CreateCommandBuffers()
{
    m_command_buffers.resize(static_cast<u32>(m_number_of_images));
    m_vk_core.CreateCommandBuffers(static_cast<u32>(m_number_of_images), m_command_buffers.data());
    APP_LOG_INFO("Command buffers created");
}

void Application::CreateMesh()
{
    CreateVertexBuffer();
    LoadTexture();
}

void Application::CreateVertexBuffer()
{
    struct Vertex {
        Vertex(const glm::vec3 &p, const glm::vec2 &t)
        {
            Pos = p;
            Tex = t;
        }

        glm::vec3 Pos;
        glm::vec2 Tex;
    };

    std::vector<Vertex> vertices = {
        Vertex({-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}), // Bottom left
        Vertex({-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}),  // Top left
        Vertex({1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}),   // Top right
        Vertex({-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}), // Bottom left
        Vertex({1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}),   // Top right
        Vertex({1.0f, -1.0f, 0.0f}, {1.0f, 0.0f})   // Bottom right
    };

    m_mesh.m_vertex_buffer_size = sizeof(vertices[0]) * vertices.size();
    m_mesh.m_vertex_buffer = m_vk_core.CreateVertexBuffer(vertices.data(), m_mesh.m_vertex_buffer_size);
}

void Application::LoadTexture()
{
    m_mesh.p_texture = new GoudaVK::VulkanTexture;
    m_vk_core.CreateTexture("assets/textures/checkerboard.png", *(m_mesh.p_texture));
}

void Application::CreateUniformBuffers() { m_uniform_buffers = m_vk_core.CreateUniformBuffers(sizeof(UniformData)); }

void Application::CreateShaders()
{
    p_vertex_shader =
        GoudaVK::CreateShaderModuleFromBinary(m_vk_core.GetDevice(), "assets/shaders/compiled/test.vert.spv");
    p_fragment_shader =
        GoudaVK::CreateShaderModuleFromBinary(m_vk_core.GetDevice(), "assets/shaders/compiled/test.frag.spv");
}

void Application::CreatePipeline()
{
    p_pipeline = std::make_unique<GoudaVK::GraphicsPipeline>(p_device, p_window, p_render_pass, p_vertex_shader,
                                                             p_fragment_shader, &m_mesh, m_number_of_images,
                                                             m_uniform_buffers, sizeof(UniformData));
}

void Application::CreateFences()
{
    if (p_vk_queue) {
        m_frame_fences.resize(p_vk_queue->GetMaxFramesInFlight(), VK_NULL_HANDLE);

        // Create fences for double buffering
        VkFenceCreateInfo fence_info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                     .flags = VK_FENCE_CREATE_SIGNALED_BIT};

        for (u32 i = 0; i < p_vk_queue->GetMaxFramesInFlight(); i++) {
            GoudaVK::CreateFence(p_device, &fence_info, nullptr, &m_frame_fences[i]);
        }
    }
}

void Application::RecordCommandBuffers()
{
    VkClearColorValue clear_colour{1.0f, 0.0f, 0.0f, 0.0f};
    VkClearValue clear_value;
    clear_value.color = clear_colour;

    VkRenderPassBeginInfo render_pass_begin_info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = p_render_pass,
        .framebuffer = VK_NULL_HANDLE, // Assigned in loop below
        .renderArea = {.offset = {.x = 0, .y = 0},
                       .extent = {.width = (u32)m_window_size.width, .height = (u32)m_window_size.height}},
        .clearValueCount = 1,
        .pClearValues = &clear_value};

    for (size_t i = 0; i < m_command_buffers.size(); i++) {
        GoudaVK::BeginCommandBuffer(m_command_buffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

        render_pass_begin_info.framebuffer = m_frame_buffers[i];

        vkCmdBeginRenderPass(m_command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        // Set viewport
        VkViewport viewport{.x = 0.0f,
                            .y = 0.0f,
                            .width = static_cast<f32>(m_window_size.width),
                            .height = static_cast<f32>(m_window_size.height),
                            .minDepth = 0.0f,
                            .maxDepth = 1.0f};

        vkCmdSetViewport(m_command_buffers[i], 0, 1, &viewport);

        // Set scissor
        VkRect2D scissor{.offset = {0, 0},
                         .extent = {static_cast<u32>(m_window_size.width), static_cast<u32>(m_window_size.height)}};

        vkCmdSetScissor(m_command_buffers[i], 0, 1, &scissor);

        p_pipeline->Bind(m_command_buffers[i], i);

        u32 VertexCount = 6;
        u32 InstanceCount = 1;
        u32 FirstVertex = 0;
        u32 FirstInstance = 0;

        vkCmdDraw(m_command_buffers[i], VertexCount, InstanceCount, FirstVertex, FirstInstance);

        vkCmdEndRenderPass(m_command_buffers[i]);

        GoudaVK::EndCommandBuffer(m_command_buffers[i]);
    }

    APP_LOG_INFO("Command buffers recorded");
}

void Application::UpdateUniformBuffer(u32 ImageIndex, f32 delta_time)
{
    static f32 rotation_speed{1.0f};
    static f32 rotation_angle{0.0f};

    glm::mat4 rotation_matrix{glm::mat4(1.0f)};

    rotation_matrix =
        glm::rotate(rotation_matrix, glm::radians(rotation_angle), glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));

    rotation_angle = fmod(rotation_angle + rotation_speed * delta_time, 360.0f);

    // std::cout << "Frame: " << ImageIndex << " Rotation: " << rotation_angle << std::endl;

    m_uniform_buffers[ImageIndex].Update(p_device, &rotation_matrix, sizeof(rotation_matrix));
}

void Application::SetupCallbacks()
{
    m_callbacks.key_callback = [this](GLFWwindow *window, int key, int scancode, int action, int mods) {
        this->OnKey(window, key, scancode, action, mods);
    };

    m_callbacks.mouse_move_callback = [this](GLFWwindow *window, f32 x_position, f32 y_position) {
        this->OnMouseMove(window, x_position, y_position);
    };

    m_callbacks.mouse_button_callback = [this](GLFWwindow *window, int button, int action, int mods) {
        this->OnMouseButton(window, button, action, mods);
    };

    m_callbacks.mouse_scroll_callback = [this](GLFWwindow *window, f32 x_offset, f32 y_offset) {
        this->OnMouseScroll(window, x_offset, y_offset);
    };

    m_callbacks.window_iconify_callback = [this](GLFWwindow *window, bool iconified) {
        this->OnWindowIconify(window, iconified);
    };

    m_callbacks.framebuffer_resized_callback = [this](GLFWwindow *window, FrameBufferSize new_size) {
        this->OnFramebufferResize(window, new_size);
    };

    GoudaVK::GLFW::set_callbacks(p_window, &m_callbacks);
}

void Application::OnKey(GLFWwindow *window, int key, int scancode, int action, int mods)
{

    if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS)) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        // TODO: Set close request flag?
    }

    if ((key == GLFW_KEY_W) && (action == GLFW_PRESS)) {
        m_audio_manager.PlaySoundEffect(m_laser_1,
                                        {Gouda::Audio::AudioEffectType::Echo, Gouda::Audio::AudioEffectType::Reverb});
    }

    if ((key == GLFW_KEY_S) && (action == GLFW_PRESS)) {
        m_audio_manager.PlaySoundEffect(m_laser_2);
    }

    if (action == GLFW_PRESS) {
        std::cout << "Key Pressed: " << key << '\n';
    }
}

void Application::OnMouseMove(GLFWwindow *window, f32 xpos, f32 ypos)
{
    // std::cout << "Mouse Moved: (" << xpos << ", " << ypos << ")\n";
}

void Application::OnMouseButton(GLFWwindow *window, int button, int action, int mods)
{
    if (action == GLFW_PRESS) {
        // std::cout << "Mouse Button Pressed: " << button << std::endl;
    }
}

void Application::OnMouseScroll(GLFWwindow *window, f32 x_offset, f32 y_offset)
{
    std::cout << "Mouse wheel scrolled: x:=" << x_offset << " y:=" << y_offset << '\n';
}

void Application::OnFramebufferResize(GLFWwindow *window, FrameBufferSize new_size)
{
    if (new_size.width == 0 || new_size.height == 0) {
        APP_LOG_INFO("Window minimized, skipping swapchain recreation");
        return; // Ignore minimized window to avoid swapchain issues
    }

    p_vk_queue->SetSwapchainInvalid(); // Mark swapchain for recreation (stops rendering)
    APP_LOG_INFO("Framebuffer resized: {}x{}, swapchain is now invalid", new_size.width, new_size.height);

    m_window_size = new_size; // Update stored window size

    vkDeviceWaitIdle(p_device); // Ensure no commands are running before destruction

    // Cleanup framebuffers
    m_vk_core.DestroyFramebuffers(m_frame_buffers);

    // Cleanup and Recreate swapchain and swapchain image views
    m_vk_core.ReCreateSwapchain();

    // Recreate framebuffers for new swapchain images
    m_frame_buffers = m_vk_core.CreateFramebuffers(p_render_pass);

    // Re-record command buffers with new framebuffers
    RecordCommandBuffers();

    APP_LOG_DEBUG("Swapchain and framebuffers recreated successfully.");

    p_vk_queue->SetSwapchainValid(); // resume rendering)
}

void Application::OnWindowIconify(GLFWwindow *window, bool iconified) { m_is_iconified = iconified; }
