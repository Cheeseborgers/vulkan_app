#include "application.hpp"

#include "math/vector.hpp"
#include "utility/timer.hpp"

#include "logger.hpp"

#include <thread>

namespace {
struct UniformData {
    gouda::math::Mat4 WVP;
};
}

Application::Application()
    : p_window{nullptr},
      p_callbacks{nullptr},
      m_settings_manager{"settings/settings.json", true, true},
      p_vk_queue{nullptr},
      m_number_of_images{},
      p_render_pass{VK_NULL_HANDLE},
      m_frame_buffer_size{0, 0},
      p_vertex_shader{VK_NULL_HANDLE},
      p_fragment_shader{VK_NULL_HANDLE},
      p_pipeline{nullptr},
      m_is_iconified{false},
      m_current_frame{0},
      m_time_settings{},
      p_ortho_camera{nullptr}
{
}

Application::~Application()
{
    APP_LOG_INFO("Cleaning up application");

    m_vk_core.DeviceWait(); // Ensure GPU is idle before cleanup

    for (auto fence : m_frame_fences) {
        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(m_vk_core.GetDevice(), fence, nullptr);
            fence = VK_NULL_HANDLE;
        }
    }

    // TODO: Decide whether to pass the vector or as is to make this cleaner
    m_vk_core.FreeCommandBuffers(static_cast<u32>(m_command_buffers.size()), m_command_buffers.data());

    m_vk_core.DestroyFramebuffers(m_frame_buffers);

    vkDestroyShaderModule(m_vk_core.GetDevice(), p_vertex_shader, nullptr);
    vkDestroyShaderModule(m_vk_core.GetDevice(), p_fragment_shader, nullptr);

    vkDestroyRenderPass(m_vk_core.GetDevice(), p_render_pass, nullptr);

    m_mesh.Destroy(m_vk_core.GetDevice());

    for (auto uniform_buffer : m_uniform_buffers) {
        uniform_buffer.Destroy(m_vk_core.GetDevice());
    }
}

void Application::Initialize()
{
    // TODO: Utilize the TimeSettings struct from Application config when make/loaded
    APP_LOG_INFO("Initializing");

    const ApplicationSettings settings{m_settings_manager.GetSettings()};

    // Create a window
    gouda::WindowConfig window_config{};
    window_config.size = settings.size;
    window_config.title = "Gouda Renderer";
    window_config.resizable = true;
    window_config.fullscreen = settings.fullscreen;
    window_config.vsync = settings.vsync;
    window_config.refresh_rate = settings.refresh_rate;
    window_config.renderer = gouda::Renderer::Vulkan;
    window_config.platform = gouda::Platform::X11;
    p_window = std::make_unique<gouda::glfw::Window>(window_config);

    // Get and cache the frame buffer size;
    m_frame_buffer_size = p_window->GetFramebufferSize();

    // Initialize Vulkan
    constexpr SemVer vulkan_api_version{1, 3, 0, 0};
    m_vk_core.Init(p_window->GetWindow(), window_config.title, vulkan_api_version, m_time_settings.vsync_mode);

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

    // Initialize orthographic camera
    f32 aspect{static_cast<float>(m_frame_buffer_size.width) / m_frame_buffer_size.height};
    p_ortho_camera =
        std::make_unique<gouda::OrthographicCamera>(-aspect, aspect, -1.0f, 1.0f, 1.0f, 2.0f,
                                                    0.5f); // left, right, bottom, top, zoom, speed, sensitivity

    // p_ortho_camera = std::make_unique<gouda::PerspectiveCamera>(60.0f, 1.777f, 0.1f, 1000.0f);
    // p_ortho_camera->SetPosition({0.0f, 0.0f, 5.0f});
    // p_ortho_camera->SetRotation(glm::vec2(0.0f, 3.1415926535f));

    // Initialize and load audio
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

    APP_LOG_DEBUG("Application initialization success");
}

void Application::Update(f32 delta_time) { p_ortho_camera->Update(delta_time); }

void Application::RenderScene(f32 delta_time)
{
    while (!p_vk_queue->IsSwapchainValid()) {
        std::this_thread::yield();
        APP_LOG_INFO("Rendering paused, waiting for valid swapchain");
    }

    vkWaitForFences(m_vk_core.GetDevice(), 1, &m_frame_fences[m_current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_vk_core.GetDevice(), 1, &m_frame_fences[m_current_frame]);

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

    gouda::utils::FrameTimer frame_timer;
    gouda::utils::FixedTimer physics_timer(m_time_settings.fixed_timestep);
    gouda::utils::GameClock game_clock;

    // Only use FPS limiter if V-Sync is disabled
    std::optional<gouda::utils::FrameRateLimiter> fps_limiter;
    if (m_time_settings.vsync_mode == gouda::vk::VSyncMode::DISABLED) {
        fps_limiter.emplace(m_time_settings.target_fps);
    }

    while (!p_window->ShouldClose()) {

        p_window->PollEvents();
        m_audio_manager.Update(); // TODO: IS this where we want to update audio? also do we pause audio when minimised?

        frame_timer.Update();
        delta_time = frame_timer.GetDeltaTime();

        // Apply time scaling
        delta_time = game_clock.ApplyTimeScale(delta_time);

        if (m_is_iconified) {
            // TODO: Hide bellow in the event handle when done
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

    m_vk_core.DeviceWait();
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
        Vertex(const gouda::math::Vec3 &p, const gouda::math::Vec2 &t)
        {
            Pos = p;
            Tex = t;
        }

        gouda::math::Vec3 Pos;
        gouda::math::Vec2 Tex;
    };

    std::vector<Vertex> vertices = {
        Vertex({-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}), // Bottom left
        Vertex({-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}),  // Top left
        Vertex({1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}),   // Top right
        Vertex({-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}), // Bottom left
        Vertex({1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}),   // Top right
        Vertex({1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}),  // Bottom right

        // Vertex({-1.0f, -1.0f, 5.0f}, {0.0f, 0.0f}), // Bottom left
        // Vertex({-1.0f, 1.0f, 5.0f}, {0.0f, 1.0f}),  // Top left
        // Vertex({1.0f, 1.0f, 5.0f}, {1.0f, 1.0f})    // Top right
    };

    m_mesh.m_vertex_buffer_size = sizeof(vertices[0]) * vertices.size();
    m_mesh.m_vertex_buffer = m_vk_core.CreateVertexBuffer(vertices.data(), m_mesh.m_vertex_buffer_size);
}

void Application::LoadTexture()
{
    m_mesh.p_texture = new gouda::vk::VulkanTexture;
    m_vk_core.CreateTexture("assets/textures/checkerboard.png", *(m_mesh.p_texture));
}

void Application::CreateUniformBuffers() { m_uniform_buffers = m_vk_core.CreateUniformBuffers(sizeof(UniformData)); }

void Application::CreateShaders()
{
    p_vertex_shader =
        gouda::vk::CreateShaderModuleFromBinary(m_vk_core.GetDevice(), "assets/shaders/compiled/test.vert.spv");
    p_fragment_shader =
        gouda::vk::CreateShaderModuleFromBinary(m_vk_core.GetDevice(), "assets/shaders/compiled/test.frag.spv");
}

void Application::CreatePipeline()
{
    p_pipeline = std::make_unique<gouda::vk::GraphicsPipeline>(
        m_vk_core.GetDevice(), p_window->GetWindow(), p_render_pass, p_vertex_shader, p_fragment_shader, &m_mesh,
        m_number_of_images, m_uniform_buffers, sizeof(UniformData));
}

void Application::CreateFences()
{
    if (p_vk_queue) {
        m_frame_fences.resize(p_vk_queue->GetMaxFramesInFlight(), VK_NULL_HANDLE);

        // Create fences for double buffering
        VkFenceCreateInfo fence_info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                     .flags = VK_FENCE_CREATE_SIGNALED_BIT};

        for (u32 i = 0; i < p_vk_queue->GetMaxFramesInFlight(); i++) {
            gouda::vk::CreateFence(m_vk_core.GetDevice(), &fence_info, nullptr, &m_frame_fences[i]);
        }
    }
}

void Application::RecordCommandBuffers()
{
    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = {{1.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};

    FrameBufferSize framebuffer_size{p_window->GetFramebufferSize()};
    VkExtent2D render_area_extent{static_cast<u32>(framebuffer_size.width), static_cast<u32>(framebuffer_size.height)};

    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = p_render_pass;
    render_pass_begin_info.framebuffer = VK_NULL_HANDLE; // Assigned in loop below
    render_pass_begin_info.renderArea = {.offset = {.x = 0, .y = 0}, .extent = render_area_extent};
    render_pass_begin_info.clearValueCount = static_cast<u32>(clear_values.size());
    render_pass_begin_info.pClearValues = clear_values.data();

    // Dynamic viewport and scissor
    VkViewport viewport{.x = 0.0f,
                        .y = 0.0f,
                        .width = static_cast<f32>(render_area_extent.width),
                        .height = static_cast<f32>(render_area_extent.height),
                        .minDepth = 0.0f,
                        .maxDepth = 1.0f};

    VkRect2D scissor{.offset = {0, 0}, .extent = render_area_extent};

    for (size_t i = 0; i < m_command_buffers.size(); i++) {
        gouda::vk::BeginCommandBuffer(m_command_buffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

        render_pass_begin_info.framebuffer = m_frame_buffers[i];

        vkCmdBeginRenderPass(m_command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        // Set viewport
        vkCmdSetViewport(m_command_buffers[i], 0, 1, &viewport);

        // Set scissor
        vkCmdSetScissor(m_command_buffers[i], 0, 1, &scissor);

        p_pipeline->Bind(m_command_buffers[i], i);

        u32 vertex_count = 6;
        u32 instance_count = 1;
        u32 first_vertex = 0;
        u32 first_instance = 0;

        vkCmdDraw(m_command_buffers[i], vertex_count, instance_count, first_vertex, first_instance);

        vkCmdEndRenderPass(m_command_buffers[i]);

        gouda::vk::EndCommandBuffer(m_command_buffers[i]);
    }

    APP_LOG_INFO("Command buffers recorded");
}

void Application::UpdateUniformBuffer(u32 image_index, f32 delta_time)
{
    // New rotation with cam
    /*
     static float rotation_speed{1.0f};
     static float rotation_angle{0.0f};
     rotation_angle = fmod(rotation_angle + rotation_speed * delta_time, 360.0f);

     glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(rotation_angle), gouda::math::Vec3(0.0f, 0.0f, 1.0f));
     glm::mat4 wvp = m_camera->GetViewProjectionMatrix() * model;
     m_uniform_buffers[image_index].Update(m_vk_core.GetDevice(), &wvp, sizeof(wvp));
    */

    // Use camera's view-projection matrix instead of rotating quad
    gouda::math::Mat4 wvp{p_ortho_camera->GetViewProjectionMatrix()};
    m_uniform_buffers[image_index].Update(m_vk_core.GetDevice(), &wvp, sizeof(wvp));
}

void Application::SetupCallbacks()
{
    p_callbacks = std::make_unique<gouda::glfw::Callbacks<GLFWwindow *>>();

    p_callbacks->key_callback = [this](GLFWwindow *window, int key, int scancode, int action, int mods) {
        this->OnKey(window, key, scancode, action, mods);
    };

    p_callbacks->mouse_move_callback = [this](GLFWwindow *window, f32 x_position, f32 y_position) {
        this->OnMouseMove(window, x_position, y_position);
    };

    p_callbacks->mouse_button_callback = [this](GLFWwindow *window, int button, int action, int mods) {
        this->OnMouseButton(window, button, action, mods);
    };

    p_callbacks->mouse_scroll_callback = [this](GLFWwindow *window, f32 x_offset, f32 y_offset) {
        this->OnMouseScroll(window, x_offset, y_offset);
    };

    p_callbacks->window_iconify_callback = [this](GLFWwindow *window, bool iconified) {
        this->OnWindowIconify(window, iconified);
    };

    p_callbacks->framebuffer_resized_callback = [this](GLFWwindow *window, FrameBufferSize new_size) {
        this->OnFramebufferResize(window, new_size);
    };

    p_callbacks->window_resized_callback = [this](GLFWwindow *window, WindowSize new_size) {
        this->OnWindowResize(window, new_size);
    };

    p_window->SetCallbacks(p_callbacks.get());
}

void Application::OnKey(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS)) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    // Camera controls
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_A:
                p_ortho_camera->SetMovementFlag(gouda::CameraMovement::MOVE_LEFT);
                break;
            case GLFW_KEY_D:
                p_ortho_camera->SetMovementFlag(gouda::CameraMovement::MOVE_RIGHT);
                break;
            case GLFW_KEY_W:
                p_ortho_camera->SetMovementFlag(gouda::CameraMovement::MOVE_UP);
                break;
            case GLFW_KEY_S:
                p_ortho_camera->SetMovementFlag(gouda::CameraMovement::MOVE_DOWN);
                break;
            case GLFW_KEY_Q:
                p_ortho_camera->SetMovementFlag(gouda::CameraMovement::ZOOM_IN);
                break;
            case GLFW_KEY_E:
                p_ortho_camera->SetMovementFlag(gouda::CameraMovement::ZOOM_OUT);
                break;
            case GLFW_KEY_SPACE:
                p_ortho_camera->Shake(0.01f, 0.5f);
                APP_LOG_INFO("Camera shake triggered");
                break;
        }
    }
    else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_A:
                p_ortho_camera->ClearMovementFlag(gouda::CameraMovement::MOVE_LEFT);
                break;
            case GLFW_KEY_D:
                p_ortho_camera->ClearMovementFlag(gouda::CameraMovement::MOVE_RIGHT);
                break;
            case GLFW_KEY_W:
                p_ortho_camera->ClearMovementFlag(gouda::CameraMovement::MOVE_UP);
                break;
            case GLFW_KEY_S:
                p_ortho_camera->ClearMovementFlag(gouda::CameraMovement::MOVE_DOWN);
                break;
            case GLFW_KEY_Q:
                p_ortho_camera->ClearMovementFlag(gouda::CameraMovement::ZOOM_IN);
                break;
            case GLFW_KEY_E:
                p_ortho_camera->ClearMovementFlag(gouda::CameraMovement::ZOOM_OUT);
                break;
        }
    }

    if (action == GLFW_PRESS) {
        APP_LOG_DEBUG("Key pressed: {}", key);
    }
}
void Application::OnMouseMove(GLFWwindow *window, f32 xpos, f32 ypos)
{
    APP_LOG_DEBUG("Mouse moved: x:={}, y:={}", xpos, ypos);
}

void Application::OnMouseButton(GLFWwindow *window, int button, int action, int mods)
{
    if (action == GLFW_PRESS) {
        APP_LOG_DEBUG("Mouse button pressed: {}", button);
    }
}

void Application::OnMouseScroll(GLFWwindow *window, f32 x_offset, f32 y_offset)
{
    APP_LOG_INFO("Mouse wheel scrolled: x:={}, y:={}", x_offset, y_offset);
    f32 zoom_delta = y_offset * 0.1f; // Adjust 0.1f for zoom sensitivity
    p_ortho_camera->AdjustZoom(zoom_delta);
}

void Application::OnFramebufferResize(GLFWwindow *window, FrameBufferSize new_size)
{
    if (new_size.width == 0 || new_size.height == 0) {
        APP_LOG_INFO("Window minimized, skipping swapchain recreation");
        return; // Ignore minimized window to avoid swapchain issues
    }

    p_vk_queue->SetSwapchainInvalid(); // Mark swapchain for recreation (stops rendering)
    APP_LOG_INFO("Framebuffer resized: {}x{}, swapchain is now invalid", new_size.width, new_size.height);

    // Cache the new frame buffer size
    m_frame_buffer_size = new_size;

    // Ensure no commands are running before destruction
    vkDeviceWaitIdle(m_vk_core.GetDevice());

    // Cleanup existing framebuffers
    m_vk_core.DestroyFramebuffers(m_frame_buffers);

    // Cleanup and Recreate swapchain and swapchain image views
    m_vk_core.ReCreateSwapchain();

    // Recreate framebuffers for new swapchain images
    m_frame_buffers = m_vk_core.CreateFramebuffers(p_render_pass);

    // Re-record command buffers with new framebuffers
    RecordCommandBuffers();

    APP_LOG_DEBUG("Swapchain and framebuffers recreated successfully.");

    // Resume rendering
    p_vk_queue->SetSwapchainValid(); // resume rendering)
}

void Application::OnWindowResize(GLFWwindow *window, WindowSize new_size)
{
    if (new_size.area() != 0) {
        m_settings_manager.SetWindowSize(new_size);
    }

    APP_LOG_INFO("Window resized: {}x{}", new_size.width, new_size.height);
}

void Application::OnWindowIconify(GLFWwindow *window, bool iconified) { m_is_iconified = iconified; }
