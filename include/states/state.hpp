#pragma once
/**
 * @file states/state.hpp
 * @author GoudaCheeseburgers
 * @date 2025-01-06
 * @brief Application module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "core/types.hpp"
#include "math/math.hpp"

struct SharedContext;
class StateStack;

class State {
public:
    using StateID = StringView;

    explicit State(SharedContext& context, StateStack &state_stack);
    virtual ~State() = default;

    [[nodiscard]] virtual StringView GetID() const = 0;

    virtual void HandleInput() = 0;
    virtual void Update(f32 delta_time) = 0;
    virtual void Render(f32 delta_time) = 0;
    virtual void OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size) = 0;

    [[nodiscard]] virtual bool IsOpaque() const { return true; }


    virtual void OnEnter() {} // TODO: Implement debug logging here
    virtual void OnExit() {}

protected:
    SharedContext& m_context;
    StateStack &m_state_stack;
    gouda::Vec2 m_framebuffer_size;
};

