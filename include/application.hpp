#pragma once

#include "audio/audio_manager.hpp"
#include "backends/common.hpp"
#include "backends/glfw/window.hpp"
#include "cameras/orthographic_camera.hpp"
#include "gouda_vk_wrapper.hpp"
#include "utility/timer.hpp"

#include "core/settings_manager.hpp" #

// TESTING REMOVE
#include "cameras/perspective_camera.hpp"

// TODO: Figure how this will work with settings and the present mode settings
struct TimeSettings {
    f32 time_scale;                  // Game speed modifier
    f32 fixed_timestep;              // Physics update rate
    f32 max_accumulator;             // Prevents physics explosion
    f32 target_fps;                  // FPS limit (ignored if V-Sync is enabled)
    gouda::vk::VSyncMode vsync_mode; // Default to normal V-Sync

    TimeSettings()
        : time_scale{1.0f},
          fixed_timestep{1.0f / 60.0f},
          max_accumulator{0.25f},
          target_fps{144.0f},
          vsync_mode{gouda::vk::VSyncMode::ENABLED}
    {
    }
};

class Application {
public:
    Application();
    ~Application();

    void Initialize();
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
    void OnWindowResize(GLFWwindow *window, WindowSize new_size);
    void OnWindowIconify(GLFWwindow *window, bool iconified);

private:
    std::unique_ptr<gouda::glfw::Window> p_window;
    std::unique_ptr<gouda::glfw::Callbacks<GLFWwindow *>> p_callbacks; // TODO: Create an input handler to hide this

    SettingsManager m_settings_manager;

    gouda::vk::VulkanCore m_vk_core;
    gouda::vk::VulkanQueue *p_vk_queue;
    int m_number_of_images;
    std::vector<VkCommandBuffer> m_command_buffers;
    VkRenderPass p_render_pass;
    std::vector<VkFramebuffer> m_frame_buffers;
    FrameBufferSize m_frame_buffer_size;

    VkShaderModule p_vertex_shader;
    VkShaderModule p_fragment_shader;

    std::unique_ptr<gouda::vk::GraphicsPipeline> p_pipeline;

    gouda::vk::SimpleMesh m_mesh;
    std::vector<gouda::vk::AllocatedBuffer> m_uniform_buffers;

    bool m_is_iconified;

    std::vector<VkFence> m_frame_fences;
    u32 m_current_frame;

    TimeSettings m_time_settings;

    std::unique_ptr<gouda::OrthographicCamera> p_ortho_camera;
    // std::unique_ptr<Gouda::PerspectiveCamera> p_ortho_camera;

    // Audio
    gouda::audio::AudioManager m_audio_manager;
    gouda::audio::SoundEffect m_laser_1;
    gouda::audio::SoundEffect m_laser_2;
    gouda::audio::MusicTrack m_music;
    gouda::audio::MusicTrack m_music2;
    gouda::audio::MusicTrack m_music3;
};
