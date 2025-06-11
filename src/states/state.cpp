/**
 * @file editor_state.cpp
 * @author GoudaCheeseburgers
 * @date 2025-11-06
 * @brief Application state module implementation
 */
#include "states/state.hpp"

#include "core/state_stack.hpp"

State::State(SharedContext &context, StateStack &state_stack)
    : m_context{context}, m_state_stack{state_stack}, m_framebuffer_size{0.0f}
{
    m_framebuffer_size = {static_cast<f32>(m_context.renderer->GetFramebufferSize().width),
                          static_cast<f32>(m_context.renderer->GetFramebufferSize().height)};
}
