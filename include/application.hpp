#pragma once

#include "audio/audio_manager.hpp"
#include "backends/common.hpp"
#include "backends/glfw/glfw_window.hpp"
#include "backends/input_handler.hpp"
#include "cameras/orthographic_camera.hpp"
#include "core/settings_manager.hpp"
#include "renderers/vulkan/vk_renderer.hpp"
#include "utils/timer.hpp"

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
    void SetupTimerSettings(const ApplicationSettings &settings);
    void SetupWindow(const ApplicationSettings &settings);
    void SetupRenderer();
    void SetupAudio(const ApplicationSettings &settings);
    void SetupCamera();
    void CreateMesh();
    void LoadTexture();
    void SetupInputSystem();

    void OnFramebufferResize(GLFWwindow *window, FrameBufferSize new_size);
    void OnWindowResize(GLFWwindow *window, WindowSize new_size);
    void OnWindowIconify(GLFWwindow *window, bool iconified);

private:
    std::unique_ptr<gouda::glfw::Window> p_window;
    std::unique_ptr<gouda::InputHandler> p_input_handler;
    SettingsManager m_settings_manager;
    gouda::vk::VulkanRenderer m_renderer;

    gouda::vk::Mesh m_mesh;

    std::vector<gouda::vk::InstanceData> m_instances;

    bool m_is_iconified;

    TimeSettings m_time_settings;

    std::unique_ptr<gouda::OrthographicCamera> p_ortho_camera;

    gouda::vk::UniformData m_uniform_data;

    // Audio
    gouda::audio::AudioManager m_audio_manager;
    // TODO: Think about storing these objects somewhere in the audio manager
    gouda::audio::SoundEffect m_laser_1;
    gouda::audio::SoundEffect m_laser_2;
    gouda::audio::MusicTrack m_music;
    gouda::audio::MusicTrack m_music2;
    gouda::audio::MusicTrack m_music3;
};
