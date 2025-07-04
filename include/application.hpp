#pragma once
/**
 * @file application.hpp
 * @author GoudaCheeseburgers
 * @date 2025-22-02
 * @brief Application module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "audio/audio_manager.hpp"
#include "backends/common.hpp"
#include "backends/glfw/glfw_window.hpp"
#include "backends/input_handler.hpp"
#include "cameras/orthographic_camera.hpp"
#include "renderers/render_data.hpp"
#include "renderers/vulkan/vk_renderer.hpp"
#include "utils/timer.hpp"

#include "core/settings_manager.hpp"
#include "core/state_stack.hpp"

// TODO: Figure how this will work with settings and the present mode settings
struct TimeSettings {
    TimeSettings()
        : time_scale{1.0f},
          fixed_timestep{1.0f / 60.0f},
          max_accumulator{0.25f},
          target_fps{144.0f},
          vsync_mode{gouda::vk::VSyncMode::Enabled}
    {
    }

    f32 time_scale;                  // Game speed modifier
    f32 fixed_timestep;              // Physics update rate
    f32 max_accumulator;             // Prevents physics explosion
    f32 target_fps;                  // FPS limit (ignored if V-Sync is enabled)
    gouda::vk::VSyncMode vsync_mode; // Default to normal V-Sync
};

class Application {
public:
    Application();
    ~Application();

    void Run();

private:
    void Update(f32 delta_time);
    void SetupTimerSettings(const ApplicationSettings &settings);
    void SetupWindow(const ApplicationSettings &settings);
    void SetupRenderer();
    void SetupAudio(const ApplicationSettings &settings);
    void SetupCamera();
    void SetCameraProjections(const gouda::Vec2 &framebuffer_size) const;
    void LoadTextures() const;
    void LoadFonts();
    void CreateSharedContext();
    void LoadInitialState();
    void SetupInputSystem();

    void OnFramebufferResize(GLFWwindow *window, FrameBufferSize new_size);
    void OnWindowResize(GLFWwindow *window, WindowSize new_size);
    void OnWindowIconify(GLFWwindow *window, bool iconified);

private:
    std::unique_ptr<gouda::glfw::Window> p_window;
    std::unique_ptr<gouda::InputHandler> p_input_handler;
    SettingsManager m_settings_manager;
    gouda::vk::Renderer m_renderer;

    std::unique_ptr<SharedContext> p_context;
    std::unique_ptr<StateStack> p_state_stack;

    bool m_is_iconified;
    FrameBufferSize m_framebuffer_size;

    TimeSettings m_time_settings;

    std::unique_ptr<gouda::OrthographicCamera> p_scene_camera;
    std::unique_ptr<gouda::OrthographicCamera> p_ui_camera;

    gouda::UniformData m_uniform_data;

    gouda::audio::AudioManager m_audio_manager;
    gouda::audio::SoundEffect m_laser_1;
    gouda::audio::SoundEffect m_laser_2;
    gouda::audio::MusicTrack m_music;
    gouda::audio::MusicTrack m_music2;
    gouda::audio::MusicTrack m_music3;
    gouda::audio::MusicTrack m_music4;
    gouda::audio::MusicTrack m_music5;
    gouda::audio::MusicTrack m_music6;

    // std::unique_ptr<Scene> p_current_scene;
};
