/**
* @file game_state.cpp
 * @author GoudaCheeseburgers
 * @date 2025-07-06
 * @brief Application game state module implementation
 */
#include "states/game_state.hpp"

GameState::GameState(SharedContext &context, StateStack &state_stack)
: State(context, state_stack)
{}

State::StateID GameState::GetID() const { return "Game state"; }

void GameState::HandleInput() {}

void GameState::Update(const f32 delta_time) {}

void GameState::Render(const f32 delta_time) {}

void GameState::OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size)
{
    m_framebuffer_size = new_framebuffer_size;
}

void GameState::OnEnter() { State::OnEnter(); }

void GameState::OnExit() { State::OnExit(); }