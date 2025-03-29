#include "application.hpp"

#include "backends/event_types.hpp"
#include "backends/glfw/glfw_backend.hpp"
#include "debug/logger.hpp"
#include "math/vector.hpp"
#include "renderers/vulkan/gouda_vk_wrapper.hpp"
#include "renderers/vulkan/vk_uniform_data.hpp"
#include "utils/timer.hpp"

Application::Application()
    : p_window{nullptr},
      p_input_handler{nullptr},
      m_settings_manager{"config/settings.json", true, true},
      m_is_iconified{false},
      m_time_settings{},
      p_ortho_camera{nullptr},
      m_uniform_data{}
{
}

Application::~Application()
{
    APP_LOG_INFO("Cleaning up application");

    m_renderer.DeviceWait(); // Ensure GPU is idle before cleanup
    m_mesh.Destroy(m_renderer.GetDevice());
}

void Application::Initialize()
{
    APP_LOG_INFO("Initializing");

    const ApplicationSettings settings{m_settings_manager.GetSettings()};
    SetupTimerSettings(settings);
    SetupWindow(settings);
    SetupRenderer();

    // Set up instance data for two quads
    m_instances = {
        {{0.0f, 0.0f, -0.9f}, 1.0f, 0.0f},    // First quad at top-left
        {{200.0f, 200.0f, 0.0f}, 1.5f, 3.0f}, // Second quad offset by 200x200 pixels
        {{500.0f, 700.0f, 0.0f}, 2.5f, 6.0f}  // Second quad offset by 200x200 pixels
    };

    SetupInputSystem();
    SetupCamera();
    SetupAudio(settings);

    APP_LOG_DEBUG("Application initialization success");
}

void Application::Update(f32 delta_time) { p_ortho_camera->Update(delta_time); }

void Application::RenderScene(f32 delta_time)
{
    m_uniform_data.WVP = p_ortho_camera->GetViewProjectionMatrix();
    m_renderer.Render(delta_time, m_uniform_data, m_instances);
}

void Application::Execute()
{
    APP_LOG_INFO("Running...");

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

    m_audio_manager.PlayMusic(true);

    while (!p_window->ShouldClose()) {

        if (m_is_iconified) {
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

    m_renderer.DeviceWait();
}

// Private -------------------------------------------------------------------------------------------------------------
void Application::SetupTimerSettings(const ApplicationSettings &settings)
{
    m_time_settings.target_fps = settings.refresh_rate;
    m_time_settings.fixed_timestep = 1.0f / settings.refresh_rate;
    m_time_settings.vsync_mode = settings.vsync ? gouda::vk::VSyncMode::ENABLED : gouda::vk::VSyncMode::DISABLED;
}

void Application::SetupWindow(const ApplicationSettings &settings)
{
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

    p_window->SetIcon("assets/textures/gouda_icon.png");
}

void Application::SetupRenderer()
{
    // Initialize Vulkan
    const SemVer vulkan_api_version{1, 3, 0, 0};
    m_renderer.Initialize(p_window->GetWindow(), "Gouda renderer", vulkan_api_version, m_time_settings.vsync_mode);

    CreateMesh();
    m_renderer.CreateUniformBuffers(sizeof(gouda::vk::UniformData));
    m_renderer.SetupPipeline(&m_mesh, "assets/shaders/compiled/test.vert.spv", "assets/shaders/compiled/test.frag.spv");
}

void Application::SetupAudio(const ApplicationSettings &settings)
{
    m_audio_manager.Initialize(settings.audio_settings.sound_volume, settings.audio_settings.music_volume);

    m_laser_1.Load("assets/audio/sound_effects/laser1.wav");
    m_laser_2.Load("assets/audio/sound_effects/laser2.wav");

    m_music2.Load("assets/audio/music_tracks/moonlight.wav");
    m_music.Load("assets/audio/music_tracks/track.mp3");
    m_music3.Load("assets/audio/music_tracks/blondie.mp3");

    m_audio_manager.QueueMusic(m_music3);
    m_audio_manager.QueueMusic(m_music2);
    m_audio_manager.QueueMusic(m_music);
}

void Application::SetupCamera()
{
    FrameBufferSize framebuffer_size{m_renderer.GetFramebufferSize()};
    p_ortho_camera = std::make_unique<gouda::OrthographicCamera>(
        0.0f, static_cast<f32>(framebuffer_size.width),  // left = 0, right = width
        static_cast<f32>(framebuffer_size.height), 0.0f, // bottom = height, top = 0
        1.0f, 100.0f, 1.0f                               // zoom, speed, sensitivity
    );
}

void Application::CreateMesh() { LoadTexture(); }

void Application::LoadTexture()
{
    m_mesh.p_texture = m_renderer.GetBufferManager()->CreateTexture("assets/textures/checkerboard.png");
}

void Application::SetupInputSystem()
{
    auto backend = std::make_unique<gouda::glfw::GLFWBackend>([this](gouda::Event e) {
        p_input_handler->QueueEvent(e); // Queue raw events directly
    });

    p_input_handler = std::make_unique<gouda::InputHandler>(std::move(backend), p_window->GetWindow());

    std::vector<gouda::InputHandler::ActionBinding> game_bindings = {
        {gouda::Key::Escape, gouda::ActionState::Pressed,
         [this]() { glfwSetWindowShouldClose(p_window->GetWindow(), GLFW_TRUE); }},
        {gouda::Key::A, gouda::ActionState::Pressed,
         [this]() {
             APP_LOG_DEBUG("Left");
             p_ortho_camera->SetMovementFlag(gouda::CameraMovement::MOVE_LEFT);
         }},
        {gouda::Key::A, gouda::ActionState::Released,
         [this]() { p_ortho_camera->ClearMovementFlag(gouda::CameraMovement::MOVE_LEFT); }},
        {gouda::Key::D, gouda::ActionState::Pressed,
         [this]() { p_ortho_camera->SetMovementFlag(gouda::CameraMovement::MOVE_RIGHT); }},
        {gouda::Key::D, gouda::ActionState::Released,
         [this]() { p_ortho_camera->ClearMovementFlag(gouda::CameraMovement::MOVE_RIGHT); }},
        {gouda::Key::W, gouda::ActionState::Pressed,
         [this]() { p_ortho_camera->SetMovementFlag(gouda::CameraMovement::MOVE_UP); }},
        {gouda::Key::W, gouda::ActionState::Released,
         [this]() { p_ortho_camera->ClearMovementFlag(gouda::CameraMovement::MOVE_UP); }},
        {gouda::Key::S, gouda::ActionState::Pressed,
         [this]() { p_ortho_camera->SetMovementFlag(gouda::CameraMovement::MOVE_DOWN); }},
        {gouda::Key::S, gouda::ActionState::Released,
         [this]() { p_ortho_camera->ClearMovementFlag(gouda::CameraMovement::MOVE_DOWN); }},
        {gouda::Key::Q, gouda::ActionState::Pressed,
         [this]() { p_ortho_camera->SetMovementFlag(gouda::CameraMovement::ZOOM_IN); }},
        {gouda::Key::Q, gouda::ActionState::Released,
         [this]() { p_ortho_camera->ClearMovementFlag(gouda::CameraMovement::ZOOM_IN); }},
        {gouda::Key::E, gouda::ActionState::Pressed,
         [this]() { p_ortho_camera->SetMovementFlag(gouda::CameraMovement::ZOOM_OUT); }},
        {gouda::Key::E, gouda::ActionState::Released,
         [this]() { p_ortho_camera->ClearMovementFlag(gouda::CameraMovement::ZOOM_OUT); }},
        {gouda::Key::Space, gouda::ActionState::Pressed, [this]() { p_ortho_camera->Shake(10.0f, 0.5f); }},
        {gouda::MouseButton::Left, gouda::ActionState::Pressed,
         []() {
             // APP_LOG_DEBUG("Left Mouse Pressed");
         }},
    };

    p_input_handler->LoadStateBindings("Game", game_bindings);
    p_input_handler->SetActiveState("Game");

    // Scroll callback
    p_input_handler->SetScrollCallback([this](double xOffset, double yOffset) {
        f32 zoomDelta = static_cast<f32>(yOffset) * 0.1f; // Match original sensitivity
        p_ortho_camera->AdjustZoom(zoomDelta);
        APP_LOG_DEBUG("Mouse wheel scrolled: xOffset={}, yOffset={}, zoomDelta={}", xOffset, yOffset, zoomDelta);
    });

    // Character input callback
    p_input_handler->SetCharCallback([](unsigned int codepoint) {
        // char c = static_cast<char>(codepoint); // Simple ASCII cast for demo
        // APP_LOG_DEBUG("Character typed: {} (codepoint={})", c, codepoint);
    });

    // Cursor enter/leave callback
    p_input_handler->SetCursorEnterCallback(
        [](bool entered) { // APP_LOG_DEBUG("Cursor {} window", entered ? "entered" : "left");
        });

    // Window focus callback
    p_input_handler->SetWindowFocusCallback([this](bool focused) {
        m_is_iconified = !focused; // Align with your existing logic
        // APP_LOG_DEBUG("Window {} focus", focused ? "gained" : "lost");
    });

    // Framebuffer resized
    p_input_handler->SetFramebufferSizeCallback([this](int width, int height) {
        FrameBufferSize new_size{width, height};
        OnFramebufferResize(p_window->GetWindow(), new_size);
    });

    // Window resized
    p_input_handler->SetWindowSizeCallback([this](int width, int height) {
        WindowSize new_size{width, height};
        OnWindowResize(p_window->GetWindow(), new_size);
    });

    // Window iconified
    p_input_handler->SetWindowIconifyCallback(
        [this](bool iconified) { OnWindowIconify(p_window->GetWindow(), iconified); });
}

void Application::OnFramebufferResize(GLFWwindow *window, FrameBufferSize new_size)
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

    p_ortho_camera->SetProjection(0.0f, static_cast<f32>(new_size.width), // left = 0, right = width
                                  static_cast<f32>(new_size.height), 0.0f);

    APP_LOG_DEBUG("Swapchain and framebuffers recreated successfully.");
}

void Application::OnWindowResize(GLFWwindow *window, WindowSize new_size)
{
    if (new_size.area() != 0) {
        m_settings_manager.SetWindowSize(new_size);
    }

    APP_LOG_INFO("Window resized: {}x{}", new_size.width, new_size.height);
}

void Application::OnWindowIconify(GLFWwindow *window, bool iconified) { m_is_iconified = iconified; }
