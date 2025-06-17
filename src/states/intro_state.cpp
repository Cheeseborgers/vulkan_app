/**
 * @file intro_state.cpp
 * @author GoudaCheeseburgers
 * @date 2025-07-06
 * @brief Application intro state module implementation
 */
#include "states/intro_state.hpp"

#include "../../include/states/editor_state.hpp"
#include "debug/logger.hpp"

#include "states/main_menu_state.hpp"

IntroState::IntroState(SharedContext &context, StateStack &state_stack)
    : State(context, state_stack, "IntroState"), m_current_time{0.0f}
{
    gouda::InstanceData background;
    background.position = {0.f, 0.f, -0.599f};
    background.size = m_framebuffer_size;
    background.apply_camera_effects = 0;
    background.texture_index = 1;
    m_quad_instances.push_back(background);

    gouda::Vec3 text_position;
    text_position.x = m_framebuffer_size.x / 2;
    text_position.y = m_framebuffer_size.y / 2;
    text_position.z = -0.1f;

    m_context.renderer->DrawText("GOUDA RENDERER", text_position, {0.0f, 1.0f, 0.0f, 1.0f}, 50.0f, 2, m_text_instances,
                                 gouda::TextAlign::Center, false);
}

void IntroState::HandleInput()
{
    // All Input is handled by input handler for intro state
}

void IntroState::Update(const f32 delta_time)
{
    m_current_time += delta_time;
    if (m_current_time >= 3.0f) { // TODO: Change to a decent time (Set for debug)
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
    const std::vector<gouda::InputHandler::ActionBinding> intro_bindings{
        {gouda::Key::Escape, gouda::ActionState::Pressed, [this] { m_context.window->Close(); }},
        {gouda::Key::Enter, gouda::ActionState::Pressed, [this] { TransitionToMainMenu(); }}};

    m_context.input_handler->LoadStateBindings(m_state_id, intro_bindings);
    m_context.input_handler->SetActiveState(m_state_id);
}

void IntroState::OnExit()
{
    m_context.input_handler->UnloadStateBindings(m_state_id);
}

void IntroState::TransitionToMainMenu() const
{
    m_context.renderer->DeviceWait();
    // m_state_stack.Replace(std::make_unique<MainMenuState>(m_context, m_state_stack));
    m_state_stack.Replace(std::make_unique<EditorState>(m_context, m_state_stack));
}