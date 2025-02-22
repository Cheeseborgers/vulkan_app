#pragma once

#include "gouda_vk_wrapper.hpp"

// TODO: Move these to cpp file
#include <glm/ext.hpp>
#include <glm/glm.hpp>

class Application {
public:
    Application(WindowSize window_size);
    ~Application();

    void Init(std::string_view application_title);

    void Update(f32 delta_time);

    void RenderScene();

    void Execute();

private:
    void CreateCommandBuffers();
    void CreateMesh();
    void CreateVertexBuffer();
    void LoadTexture();

    struct UniformData { // TODO: Move this elsewhere
        glm::mat4 WVP;
    };

    void CreateUniformBuffers();
    void CreateShaders();
    void CreatePipeline();
    void RecordCommandBuffers();
    void UpdateUniformBuffer(u32 ImageIndex);
    void SetupCallbacks();

    // FIXME: Testing
    void RecreateSwapchain(FrameBufferSize new_size);
    void RecreateFramebuffers();

    // Callback handlers
    void OnKey(GLFWwindow *window, int key, int scancode, int action, int mods);
    void OnMouseMove(GLFWwindow *window, f32 xpos, f32 ypos);
    void OnMouseButton(GLFWwindow *window, int button, int action, int mods);
    void OnMouseScroll(GLFWwindow *window, f32 x_offset, f32 y_offset);
    void OnFramebufferResize(GLFWwindow *window, FrameBufferSize new_size);
    void OnWindowIconify(GLFWwindow *window, bool iconified);

private:
    GLFWwindow *p_window;
    WindowSize m_window_size;
    VkDevice p_device;
    GoudaVK::VulkanCore m_vk_core;
    GoudaVK::VulkanQueue *p_vk_queue;
    int m_number_of_images;
    std::vector<VkCommandBuffer> m_command_buffers;
    VkRenderPass p_render_pass;
    std::vector<VkFramebuffer> m_frame_buffers;

    VkShaderModule p_vertex_shader;
    VkShaderModule p_fragment_shader;

    // GoudaVK::GraphicsPipeline *p_pipeline; // TODO: Think about how we handle this pointer
    std::unique_ptr<GoudaVK::GraphicsPipeline> p_pipeline;

    GoudaVK::SimpleMesh m_mesh;
    std::vector<GoudaVK::AllocatedBuffer> m_uniform_buffers;

    GoudaVK::GLFW::Callbacks m_callbacks;

    bool m_is_iconified;

    std::vector<VkFence> m_frame_fences;
    u32 m_current_frame;
    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
};
