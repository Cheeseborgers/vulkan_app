#pragma once

#include "backends/glfw_window.hpp"
#include "gouda_vk_wrapper.hpp"

#include "audio/audio_manager.hpp"
#include "utility/timer.hpp"

// TODO: Figure how this will intergrate with app config, window config and such
struct TimeSettings {
    f32 time_scale;                // Game speed modifier
    f32 fixed_timestep;            // Physics update rate
    f32 max_accumulator;           // Prevents physics explosion
    f32 target_fps;                // FPS limit (ignored if V-Sync is enabled)
    GoudaVK::VSyncMode vsync_mode; // Default to normal V-Sync

    TimeSettings()
        : time_scale{1.0f},
          fixed_timestep{1.0f / 60.0f},
          max_accumulator{0.25f},
          target_fps{144.0f},
          vsync_mode{GoudaVK::VSyncMode::ENABLED}
    {
    }
};

// TODO: Load and save the application config from file, with safe defaults (ignore renderer, platform and title for
// now)
// TODO: Create an application config and construct the window config from that so we can hold data such as the
// renderer. we dont require window size as the window should return those

class Application {
public:
    Application();
    ~Application();

    void Init(const Gouda::WindowConfig &config);
    void Update(f32 delta_time);
    void RenderScene(f32 delta_time);
    void Execute();

private:
    void CreateCommandBuffers();
    void CreateMesh();
    void CreateVertexBuffer();
    void LoadTexture();
    void CreateUniformBuffers();
    void CreateShaders();
    void CreatePipeline();
    void CreateFences();
    void RecordCommandBuffers();
    void UpdateUniformBuffer(u32 image_index, f32 delta_time);
    void SetupCallbacks();

    // Callback handlers
    void OnKey(GLFWwindow *window, int key, int scancode, int action, int mods);
    void OnMouseMove(GLFWwindow *window, f32 xpos, f32 ypos);
    void OnMouseButton(GLFWwindow *window, int button, int action, int mods);
    void OnMouseScroll(GLFWwindow *window, f32 x_offset, f32 y_offset);
    void OnFramebufferResize(GLFWwindow *window, FrameBufferSize new_size);
    void OnWindowIconify(GLFWwindow *window, bool iconified);

private:
    std::unique_ptr<Gouda::GLFW::Window> p_window;
    std::unique_ptr<Gouda::GLFW::Callbacks<GLFWwindow *>> p_callbacks; // TODO: Create an input handler to hide this

    GoudaVK::VulkanCore m_vk_core;
    GoudaVK::VulkanQueue *p_vk_queue;
    int m_number_of_images;
    std::vector<VkCommandBuffer> m_command_buffers;
    VkRenderPass p_render_pass;
    std::vector<VkFramebuffer> m_frame_buffers;

    VkShaderModule p_vertex_shader;
    VkShaderModule p_fragment_shader;

    std::unique_ptr<GoudaVK::GraphicsPipeline> p_pipeline;

    GoudaVK::SimpleMesh m_mesh;
    std::vector<GoudaVK::AllocatedBuffer> m_uniform_buffers;

    bool m_is_iconified;

    std::vector<VkFence> m_frame_fences;
    u32 m_current_frame;

    TimeSettings m_time_settings;

    // Audio
    Gouda::Audio::AudioManager m_audio_manager;
    Gouda::Audio::SoundEffect m_laser_1;
    Gouda::Audio::SoundEffect m_laser_2;
    Gouda::Audio::MusicTrack m_music;
    Gouda::Audio::MusicTrack m_music2;
    Gouda::Audio::MusicTrack m_music3;
};
