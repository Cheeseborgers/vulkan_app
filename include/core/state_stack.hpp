#pragma once
/**
 * @file core/state_stack.hpp
 * @author GoudaCheeseburgers
 * @date 2025-01-06
 * @brief Application scene module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <memory>
#include <optional>

#include "backends/input_handler.hpp"
#include "backends/glfw/glfw_window.hpp"
#include "cameras/orthographic_camera.hpp"
#include "containers/small_vector.hpp"
#include "core/types.hpp"
#include "renderers/vulkan/vk_renderer.hpp"
#include "renderers/vulkan/vk_texture_manager.hpp"

#include "settings_manager.hpp"
#include "states/state.hpp"

struct SharedContext {
    gouda::vk::Renderer *renderer;
    gouda::glfw::Window *window;

    gouda::InputHandler *input_handler;
    gouda::vk::TextureManager *texture_manager;
    SettingsManager *settings_manager;

    gouda::OrthographicCamera *scene_camera;
    gouda::OrthographicCamera *ui_camera;

    gouda::UniformData *uniform_data;
};

class StateStack {
public:
    enum class Action : u8 { Push, Pop, Replace };

    explicit StateStack() = default;

    ~StateStack();

    void Push(std::unique_ptr<State> state);
    void Pop();
    void Replace(std::unique_ptr<State> state);

    void HandleInput();
    void Update(f32 delta_time);
    void Render(f32 delta_time) const;
    void OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size);
    void ApplyPendingChanges();

    [[nodiscard]] bool IsEmpty() const { return m_states.empty(); }
    [[nodiscard]] State::StateID GetTopStateID() const;

private:
    struct PendingChange {
        Action action;
        std::optional<std::unique_ptr<State>> state;
    };

private:
    gouda::Vector<std::unique_ptr<State>> m_states;
    gouda::Vector<PendingChange> m_pending_changes;
};
