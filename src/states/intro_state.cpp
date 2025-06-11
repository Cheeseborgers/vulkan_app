/**
 * @file intro_state.cpp
 * @author GoudaCheeseburgers
 * @date 2025-07-06
 * @brief Application intro state module implementation
 */
#include "states/intro_state.hpp"

#include "debug/logger.hpp"
#include "states/editor_state.hpp"

#include "states/main_menu_state.hpp"

IntroState::IntroState(SharedContext &context, StateStack &state_stack)
    : State(context, state_stack), m_current_time{0.0f}
{
    gouda::Vec2 framebuffer_size;
    framebuffer_size.x = static_cast<f32>(m_context.renderer->GetFramebufferSize().width);
    framebuffer_size.y = static_cast<f32>(m_context.renderer->GetFramebufferSize().height);

    gouda::InstanceData background{};
    background.position = {0.f, 0.f, -0.599f};
    background.size = framebuffer_size;
    background.apply_camera_effects = 0;
    background.texture_index = 1;
    m_quad_instances.push_back(background);

    gouda::Vec3 text_position;
    text_position.x = framebuffer_size.x / 2;
    text_position.y = framebuffer_size.y / 2;
    text_position.z = -0.1f;

    m_context.renderer->DrawText("GOUDA RENDERER", text_position, {0.0f, 1.0f, 0.0f, 1.0f}, 50.0f, 2, m_text_instances,
                                 gouda::TextAlign::Center, false);
}

State::StateID IntroState::GetID() const { return "IntroState"; }

void IntroState::HandleInput()
{
    // Allow skipping the intro with a key press
    if (m_context.input_handler->IsKeyPressed(gouda::Key::Enter)) {
        TransitionToMainMenu();
    }
}

void IntroState::Update(const f32 delta_time)
{
    m_current_time += delta_time;
    if (m_current_time >= 3.0f) {
        TransitionToMainMenu();
    }
}

void IntroState::Render(const f32 delta_time)
{
    m_context.renderer->Render(delta_time, *m_context.uniform_data, m_quad_instances, m_text_instances,
                               m_particles_instances);
}
void IntroState::OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size)
{
    m_framebuffer_size = new_framebuffer_size;
}

void IntroState::OnEnter()
{
    APP_LOG_INFO("Entered IntroState");
    State::OnEnter();

    // TODO: Set up keybinds/input
}

void IntroState::OnExit()
{
    APP_LOG_INFO("Exiting IntroState");
    State::OnExit();

    // TODO: Unset keybinds/input
}

void IntroState::TransitionToMainMenu() const
{
    m_context.renderer->DeviceWait();
    //m_state_stack.Replace(std::make_unique<MainMenuState>(m_context, m_state_stack));
    m_state_stack.Replace(std::make_unique<EditorState>(m_context, m_state_stack));
}