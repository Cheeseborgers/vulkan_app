#include "application.hpp"

// TODO: Move these to cpp file
#include <glm/ext.hpp>
#include <glm/glm.hpp>

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

    p_window = GoudaVK::GLFW::vulkan_init(m_window_size, application_title, false).value_or(nullptr);

    m_vk_core.Init(p_window, application_title, {1, 3, 0, 0});

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
}

void Application::Update(f32 delta_time) {}

void Application::RenderScene()
{
    vkWaitForFences(p_device, 1, &m_frame_fences[m_current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(p_device, 1, &m_frame_fences[m_current_frame]);

    u32 image_index = p_vk_queue->AcquireNextImage(m_current_frame);
    UpdateUniformBuffer(image_index);

    p_vk_queue->Submit(m_command_buffers[image_index], m_current_frame, m_frame_fences[m_current_frame]);
    p_vk_queue->Present(image_index, m_current_frame);

    m_current_frame = (m_current_frame + 1) % p_vk_queue->GetMaxFramesInFlight();
}

void Application::Execute()
{
    constexpr f32 fixed_timestep = 1.0f / 60.0f; // Physics updates at 60Hz

    auto previous_time = SteadyClock::now();
    f32 accumulator = 0.0f;

    constexpr Milliseconds sleep_duration(1); // Small sleep to reduce CPU usage

    while (!glfwWindowShouldClose(p_window)) {
        auto current_time = SteadyClock::now();
        std::chrono::duration<f32> frame_time = current_time - previous_time;
        previous_time = current_time;

        f32 dt = frame_time.count();
        accumulator += dt;

        glfwPollEvents();

        if (m_is_iconified) {
            std::this_thread::sleep_for(sleep_duration);
            continue; // Skip rendering etc.. while minimized
        }

        while (accumulator >= fixed_timestep) {
            Update(fixed_timestep);
            accumulator -= fixed_timestep;
        }

        RenderScene(); // Render at variable FPS
    }

    vkDeviceWaitIdle(p_device); // Wait for GPU to finish before destroying window
    glfwDestroyWindow(p_window);
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
                       .extent = {.width = m_window_size.width, .height = m_window_size.height}},
        .clearValueCount = 1,
        .pClearValues = &clear_value};

    for (size_t i = 0; i < m_command_buffers.size(); i++) {

        GoudaVK::BeginCommandBuffer(m_command_buffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

        render_pass_begin_info.framebuffer = m_frame_buffers[i];

        vkCmdBeginRenderPass(m_command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

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

void Application::UpdateUniformBuffer(u32 ImageIndex)
{
    static float rotation_angle = 0.0f;
    glm::mat4 rotation_matrix = glm::mat4(1.0f);

    rotation_matrix =
        glm::rotate(rotation_matrix, glm::radians(rotation_angle), glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));

    rotation_angle += 0.001f; // Increment the angle for animation

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
    // Step 1: Mark the window size as changed
    m_window_size = new_size;

    // Step 2: Recreate swapchain
    // RecreateSwapchain(new_size);

    // Step 3: Recreate associated framebuffers
    // RecreateFramebuffers();

    // Step 4: Update any dependent uniform buffers (like projection matrices)
    // UpdateProjectionMatrix(new_size.width, new_size.height);

    // Step 5: Re-record command buffers, as the framebuffer and swapchain have changed
    // RecordCommandBuffers();
}

void Application::OnWindowIconify(GLFWwindow *window, bool iconified) { m_is_iconified = iconified; }
