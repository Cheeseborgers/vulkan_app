#pragma once
/**
 * @file states/main_menu_state.hpp
 * @author GoudaCheeseburgers
 * @date 2025-07-06
 * @brief Application main menu state module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "core/state_stack.hpp"
#include "state.hpp"

class MainMenuState final : public State {
public:
    explicit MainMenuState(SharedContext &context, StateStack &state_stack);
    ~MainMenuState() override = default;

    void HandleInput() override;
    void Update(f32 delta_time) override;
    void Render(f32 delta_time) override;
    void OnFrameBufferResize(const gouda::Vec2 &new_framebuffer_size) override;
    [[nodiscard]] bool IsOpaque() const override { return false; }

    void OnEnter() override;
    void OnExit() override;

private:
    struct ButtonBounds {
        f32 x, y, width, height;
    };

private:
    std::vector<gouda::InstanceData> m_quad_instances;
    std::vector<gouda::TextData> m_text_instances;
    std::vector<gouda::ParticleData> m_particles_instances;

    std::vector<ButtonBounds> m_button_bounds; // For input detection
};
