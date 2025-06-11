/**
* @file state_stack.cpp
 * @author GoudaCheeseburgers
 * @date 2025-01-6
 * @brief Application state stack module implementation
 */
#include "core/state_stack.hpp"

#include <ranges>

#include "states/state.hpp"

StateStack::~StateStack()
{
    if (!m_states.empty()) {
        for (const auto& state : m_states) {
            state->OnExit();
        }
    }
}

void StateStack::Push(std::unique_ptr<State> state){

    if (!state) {
        throw std::invalid_argument("Cannot push null state");
    }

    m_pending_changes.push_back({Action::Push, std::move(state)});
}

void StateStack::Pop() {
    if (!m_states.empty()) {
        m_pending_changes.push_back({Action::Pop, std::nullopt});
    }
}

void StateStack::Replace(std::unique_ptr<State> state)  {
    if (!state) {
        throw std::invalid_argument("Cannot replace with null state");
    }

    m_pending_changes.push_back({Action::Replace, std::move(state)});
}

void StateStack::HandleInput() {
    for (const auto & m_state : std::ranges::reverse_view(m_states)) {
        m_state->HandleInput();
        if (m_state->IsOpaque()) {
            break;
        }
    }
}

void StateStack::Update(const f32 delta_time) {
    for (const auto & m_state : std::ranges::reverse_view(m_states)) {
        m_state->Update(delta_time);
        if (m_state->IsOpaque()) {
            break;
        }
    }
}

void StateStack::ApplyPendingChanges() {
    for (auto &[action, state] : m_pending_changes) {
        switch (action) {
            case Action::Push:
                if (state) {
                    m_states.push_back(std::move(*state));
                    m_states.back()->OnEnter();
                }
                break;
            case Action::Pop:
                if (!m_states.empty()) {
                    m_states.back()->OnExit();
                    m_states.pop_back();
                }
                break;
            case Action::Replace:
                if (!m_states.empty()) {
                    m_states.back()->OnExit();
                    m_states.pop_back();
                }
                if (state) {
                    m_states.push_back(std::move(*state));
                    m_states.back()->OnEnter();
                }
                break;
        }
    }

    m_pending_changes.clear();
}

void StateStack::Render(const f32 delta_time) const
{
    for (const auto &m_state : m_states) {
        m_state->Render(delta_time);
        if (m_state->IsOpaque()) {
            break;
        }
    }
}
void StateStack::OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size)
{
    for (const auto &m_state : std::ranges::reverse_view(m_states)) {
        m_state->OnFrameBufferResize(new_framebuffer_size);
    }
}

State::StateID StateStack::GetTopStateID() const {
    return m_states.empty() ? "" : m_states.back()->GetID();
}

