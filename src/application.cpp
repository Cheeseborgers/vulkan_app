/**
 * @file application.cpp
 * @author GoudaCheeseburgers
 * @date 2025-22-02
 * @brief Application module implementation
 */
#include "application.hpp"

#include <utility>

#include "backends/event_types.hpp"
#include "backends/glfw/glfw_backend.hpp"
#include "debug/logger.hpp"
#include "math/vector.hpp"
#include "utils/timer.hpp"

#include "core/constants.hpp"
#include "core/state_stack.hpp"
#include "states/intro_state.hpp"

Application::Application()
    : p_window{nullptr},
      p_input_handler{nullptr},
      m_settings_manager{"config/settings.json", true, true},
      p_context{nullptr},
      p_state_stack{nullptr},
      m_is_iconified{false},
      m_framebuffer_size{0, 0},
      p_scene_camera{nullptr}
{
    APP_LOG_INFO("Initializing");

    const ApplicationSettings settings{m_settings_manager.GetSettings()};
    SetupTimerSettings(settings);
    SetupWindow(settings);
    SetupRenderer();

    m_framebuffer_size = m_renderer.GetFramebufferSize();

    SetupInputSystem();
    SetupCamera();
    SetupAudio(settings);

    LoadTextures();
    LoadFonts();

    CreateSharedContext();
    LoadInitialState();

    APP_LOG_DEBUG("Application initialization success");
}

Application::~Application()
{
    APP_LOG_INFO("Cleaning up application");
    m_renderer.DeviceWait(); // Ensure GPU is idle before cleanup
}

void Application::Update(const f32 delta_time)
{
    p_scene_camera->Update(delta_time);
    p_ui_camera->Update(delta_time);

    m_uniform_data.wvp = p_scene_camera->GetViewProjectionMatrix();
    m_uniform_data.wvp_static = p_ui_camera->GetViewProjectionMatrix();
}

void Application::Run()
{
    APP_LOG_INFO("Running...");

    f32 delta_time{0.0f};

    gouda::utils::FrameTimer frame_timer;
    gouda::utils::FixedTimer physics_timer(m_time_settings.fixed_timestep);
    gouda::utils::GameClock game_clock;

    // Only use FPS limiter if V-Sync is disabled
    std::optional<gouda::utils::FrameRateLimiter> fps_limiter;
    if (m_time_settings.vsync_mode == gouda::vk::VSyncMode::Disabled) {
        fps_limiter.emplace(m_time_settings.target_fps);
    }

    m_audio_manager.PlayMusic(true);

    while (!p_window->ShouldClose()) {

        if (m_is_iconified) {
            constexpr f64 sleep_duration{0.001};
            gouda::glfw::wait_events(sleep_duration); // Avoid busy waiting
            p_input_handler->Update();
            m_audio_manager.Update();
            continue; // Skip rendering while minimized
        }

        p_input_handler->Update();
        m_audio_manager.Update();

        frame_timer.Update();
        delta_time = frame_timer.GetDeltaTime();
        delta_time = game_clock.ApplyTimeScale(delta_time); // Apply time scaling

        p_state_stack->HandleInput(); // Handle state input

        // Update physics at a fixed timestep
        physics_timer.UpdateAccumulator(delta_time);
        while (physics_timer.ShouldUpdate()) {
            p_state_stack->Update(physics_timer.GetFixedTimeStep());
            physics_timer.Advance();
        }

        Update(delta_time);
        p_state_stack->Render(delta_time);

        p_state_stack->ApplyPendingChanges(); // Apply any changes to the state stack

        // Apply FPS limiter only if V-Sync is off
        if (fps_limiter) {
            fps_limiter->Limit(delta_time);
        }
    }

    m_renderer.DeviceWait();
}

// Private ---------------------------------------------------------------------------------------------
void Application::SetupTimerSettings(const ApplicationSettings &settings)
{
    m_time_settings.target_fps = settings.refresh_rate;
    m_time_settings.fixed_timestep = 1.0f / static_cast<f32>(settings.refresh_rate);
    m_time_settings.vsync_mode = settings.vsync ? gouda::vk::VSyncMode::Enabled : gouda::vk::VSyncMode::Disabled;
}

void Application::SetupWindow(const ApplicationSettings &settings)
{
    gouda::WindowConfig window_config{};
    window_config.size = settings.size;
    window_config.title = "Gouda renderer";
    window_config.resizable = true;
    window_config.fullscreen = settings.fullscreen;
    window_config.vsync = settings.vsync;
    window_config.refresh_rate = settings.refresh_rate;
    window_config.renderer = gouda::Renderer::Vulkan;
    window_config.platform = gouda::Platform::X11;

    p_window = std::make_unique<gouda::glfw::Window>(window_config);

    p_window->SetIcon(filepath::application_icon);
}

void Application::SetupRenderer()
{
    // Initialize Vulkan
    m_renderer.Initialize(p_window->GetWindow(), "Gouda renderer", SemVer{1, 4, 0, 0}, m_time_settings.vsync_mode);

    m_renderer.CreateUniformBuffers(sizeof(gouda::UniformData));

    m_renderer.SetupPipelines(filepath::quad_vertex_shader, filepath::quad_frag_shader, filepath::text_vertex_shader,
                              filepath::text_frag_shader, filepath::particle_vertex_shader,
                              filepath::particle_frag_shader, filepath::particle_compute_shader);
}

void Application::SetupAudio(const ApplicationSettings &settings)
{
    m_audio_manager.Initialize(settings.audio_settings.sound_volume, settings.audio_settings.music_volume);

    // TODO: Consider storing these filepaths as constant strings for easier change and locating
    m_laser_1.Load("assets/audio/sound_effects/laser1.wav");
    m_laser_2.Load("assets/audio/sound_effects/laser2.wav");

    m_music2.Load("assets/audio/music_tracks/moonlight.wav");
    m_music.Load("assets/audio/music_tracks/track.mp3");
    m_music3.Load("assets/audio/music_tracks/blondie.mp3");
    m_music4.Load("assets/audio/music_tracks/half.mp3");
    m_music5.Load("assets/audio/music_tracks/the.mp3");
    m_music6.Load("assets/audio/music_tracks/robin.mp3");

    m_audio_manager.QueueMusic(m_music3);
    m_audio_manager.QueueMusic(m_music2);
    m_audio_manager.QueueMusic(m_music);
    m_audio_manager.QueueMusic(m_music4);
    m_audio_manager.QueueMusic(m_music5);
    m_audio_manager.QueueMusic(m_music6);
}

void Application::SetupCamera()
{
    const FrameBufferSize framebuffer_size{m_renderer.GetFramebufferSize()};
    const f32 width{static_cast<f32>(framebuffer_size.width)};
    const f32 height{static_cast<f32>(framebuffer_size.height)};

    p_scene_camera = std::make_unique<gouda::OrthographicCamera>(0.0f, width,  // left = 0, right = width
                                                                 height, 0.0f, // bottom = height, top = 0
                                                                 -1.0f, 1.0f, 1.0f, 100.0f,
                                                                 1.0f // near, far, zoom, speed, sensitivity
    );

    p_ui_camera =
        std::make_unique<gouda::OrthographicCamera>(0.0f, width,                  // left = 0, right = width
                                                    height, 0.0f,                 // bottom = height, top = 0
                                                    -1.0f, 1.0f, 1.0f, 0.0f, 0.0f // near, far, zoom, speed, sensitivity
        );
}
void Application::SetCameraProjections(const gouda::Vec2 &framebuffer_size) const
{
    p_scene_camera->SetProjection(0.0f, framebuffer_size.x, // left = 0, right = width
                                  framebuffer_size.y, 0.0f);

    p_ui_camera->SetProjection(0.0f, framebuffer_size.x, // left = 0, right = width
                               framebuffer_size.y, 0.0f);
}

void Application::LoadTextures() const
{
    // TODO: Consider storing these filepaths as constant strings for easier change and locating
    u32 checkerboard_id = m_renderer.LoadSingleTexture("assets/textures/checkerboard.png");
    u32 checkerboard_2_id = m_renderer.LoadSingleTexture("assets/textures/checkerboard2.png");
    u32 checkerboard_3_id = m_renderer.LoadSingleTexture("assets/textures/checkerboard3.png");
    u32 checkerboard_4_id = m_renderer.LoadSingleTexture("assets/textures/checkerboard4.png");

    APP_LOG_DEBUG("Loaded textures: {},{},{},{}", checkerboard_id, checkerboard_2_id, checkerboard_3_id,
                  checkerboard_4_id);

    u32 atlas_id{m_renderer.LoadAtlasTexture(filepath::texture_atlas, filepath::texture_atlas_metadata)};
    APP_LOG_DEBUG("Atlas ID: {}", atlas_id);
}

void Application::LoadFonts()
{
    m_renderer.LoadMSDFFont(filepath::primary_font_atlas, filepath::primary_font_metadata);
    m_renderer.LoadMSDFFont(filepath::secondary_font_atlas, filepath::secondary_font_metadata);
}
void Application::CreateSharedContext()
{
    p_context = std::make_unique<SharedContext>();
    p_context->renderer = &m_renderer;
    p_context->window = p_window.get();
    p_context->input_handler = p_input_handler.get();
    p_context->texture_manager = m_renderer.GetTextureManager();
    p_context->scene_camera = p_scene_camera.get();
    p_context->ui_camera = p_ui_camera.get();
    p_context->uniform_data = &m_uniform_data;
}

void Application::LoadInitialState()
{
    if (!p_context) {
        APP_LOG_ERROR("Failed to load initial state as shared context is nullptr.");
        // TODO: Throw here.
        return;
    }

    p_state_stack = std::make_unique<StateStack>();
    p_state_stack->Push(std::make_unique<IntroState>(*p_context, *p_state_stack));
    p_state_stack->ApplyPendingChanges();
}

void Application::SetupInputSystem()
{
    auto backend = std::make_unique<gouda::glfw::GLFWBackend>([this](const gouda::Event &event) {
        p_input_handler->QueueEvent(event); // Queue raw events directly
    });

    p_input_handler = std::make_unique<gouda::InputHandler>(std::move(backend), p_window->GetWindow());

    // Scroll callback
    p_input_handler->SetScrollCallback([this]([[maybe_unused]] const f64 xOffset, const f64 yOffset) {
        const f32 zoom_delta{static_cast<f32>(yOffset) * 0.1f};
        p_scene_camera->AdjustZoom(zoom_delta);
    });

    // Character input callback
    p_input_handler->SetCharCallback([]([[maybe_unused]] unsigned int codepoint) {
        // char c = static_cast<char>(codepoint); // Simple ASCII cast for demo
        // APP_LOG_DEBUG("Character typed: {} (codepoint={})", c, codepoint);
    });

    // Cursor enter/leave callback
    p_input_handler->SetCursorEnterCallback(
        []([[maybe_unused]] bool entered) { // APP_LOG_DEBUG("Cursor {} window", entered ? "entered" : "left");
        });

    // Window focus callback
    p_input_handler->SetWindowFocusCallback([this](const bool focused) {
        m_is_iconified = !focused; // Align with your existing logic
        // APP_LOG_DEBUG("Window {} focus", focused ? "gained" : "lost");
    });

    // Framebuffer resized
    p_input_handler->SetFramebufferSizeCallback([this](const int width, const int height) {
        m_framebuffer_size = FrameBufferSize{width, height};
        OnFramebufferResize(p_window->GetWindow(), m_framebuffer_size);
    });

    // Window resized
    p_input_handler->SetWindowSizeCallback([this](const int width, const int height) {
        const WindowSize new_size{width, height};
        OnWindowResize(p_window->GetWindow(), new_size);
    });

    // Window iconified
    p_input_handler->SetWindowIconifyCallback(
        [this](const bool iconified) { OnWindowIconify(p_window->GetWindow(), iconified); });
}

void Application::OnFramebufferResize([[maybe_unused]] GLFWwindow *window, FrameBufferSize new_size)
{
    if (new_size.width == 0 || new_size.height == 0) {
        APP_LOG_INFO("Window minimized, skipping swapchain recreation");
        return; // Ignore minimized window to avoid swapchain issues
    }

    APP_LOG_INFO("Framebuffer resized: {}x{}, swapchain is now invalid", new_size.width, new_size.height);

    // Ensure no commands are running before destruction
    m_renderer.DeviceWait();

    // Cleanup and Recreate swapchain, swapchain image views, and depth resources.
    m_renderer.ReCreateSwapchain();

    const gouda::Vec2 float_size{static_cast<f32>(new_size.width), static_cast<f32>(new_size.height)};
    SetCameraProjections(float_size);

    p_state_stack->OnFrameBufferResize(float_size); // Resize the current states

    APP_LOG_DEBUG("Swapchain and framebuffers recreated successfully. States updated.");
}

void Application::OnWindowResize([[maybe_unused]] GLFWwindow *window, WindowSize new_size)
{
    if (new_size.area() != 0) {
        m_settings_manager.SetWindowSize(new_size);
    }

    APP_LOG_INFO("Window resized: {}x{}", new_size.width, new_size.height);
}

void Application::OnWindowIconify([[maybe_unused]] GLFWwindow *window, const bool iconified)
{
    m_is_iconified = iconified;
}