/**
 * @file state.cpp
 * @author GoudaCheeseburgers
 * @date 2025-11-06
 * @brief Application state module implementation
 */
#include "states/state.hpp"

#include "core/state_stack.hpp"

State::State(SharedContext &context, StateStack &state_stack, StringView identifier)
    : m_context{context}, m_state_stack{state_stack}, m_framebuffer_size{0.0f}, m_state_id{identifier}
{
    m_framebuffer_size = {static_cast<f32>(m_context.renderer->GetFramebufferSize().width),
                          static_cast<f32>(m_context.renderer->GetFramebufferSize().height)};
}
