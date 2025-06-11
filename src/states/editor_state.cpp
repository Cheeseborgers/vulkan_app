/**
 * @file editor_state.cpp
 * @author GoudaCheeseburgers
 * @date 2025-07-06
 * @brief Application editor state module implementation
 */
#include "states/editor_state.hpp"

#include "debug/logger.hpp"

EditorState::EditorState(SharedContext &context, StateStack &state_stack)
    : State(context, state_stack),
      m_side_panel{context, {0.2f, 1.0f}, {10.0f, 0.0f}, {0.2f, 0.2f, 0.2f, 1.0f}, "PANEL", 1, {0.3f, 0.3f, 0.3f, 1.0f},
                   50.0F},
      m_ui_manager{context}
{
    m_quad_instances.reserve(1000);
    m_text_instances.reserve(1000);
}

State::StateID EditorState::GetID() const { return "EditorState"; }

void EditorState::HandleInput()
{
    static bool was_pressed = false;
    const bool is_pressed = m_context.input_handler->IsKeyPressed(gouda::Key::P);
    if (is_pressed && !was_pressed) {
        m_side_panel.Toggle();
    }
    was_pressed = is_pressed;
}
void EditorState::Update(const f32 delta_time) { m_side_panel.Update(delta_time); }
void EditorState::Render(const f32 delta_time)
{
    // Do other render stuff
    m_side_panel.Render(m_quad_instances, m_text_instances);

    // Render all the stuff
    m_context.renderer->Render(delta_time, *m_context.uniform_data, m_quad_instances, m_text_instances,
                               m_particles_instances);

    m_quad_instances.clear();
    m_text_instances.clear();
}
void EditorState::OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size)
{
    m_framebuffer_size = new_framebuffer_size;

    m_side_panel.OnFramebufferResize(new_framebuffer_size);
}

void EditorState::OnEnter() { State::OnEnter(); }
void EditorState::OnExit() { State::OnExit(); }