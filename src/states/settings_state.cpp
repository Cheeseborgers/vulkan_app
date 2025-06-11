#include "states/settings_state.hpp"

SettingsState::SettingsState(SharedContext &context, StateStack &state_stack)
: State(context, state_stack)
{}

State::StateID SettingsState::GetID() const { return "Game state"; }

void SettingsState::HandleInput() {}

void SettingsState::Update(const f32 delta_time) {}

void SettingsState::Render(const f32 delta_time) {}

void SettingsState::OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size)
{
    m_framebuffer_size = new_framebuffer_size;
}

void SettingsState::OnEnter() { State::OnEnter(); }

void SettingsState::OnExit() { State::OnExit(); }
