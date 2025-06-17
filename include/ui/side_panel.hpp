#pragma once
/**
 * @file ui/side_panel.hpp
 * @author GoudaCheeseburgers
 * @date 2025-08-06
 * @brief Application ui side panel module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */

#include "core/state_stack.hpp"
#include "core/types.hpp"
#include "math/easing.hpp"

enum class PanelSide : u8 { Left, Right };

class SidePanel {
public:
    SidePanel(SharedContext &shared_context, const gouda::Vec2 &size, const gouda::Vec2 &padding,
              const gouda::Colour<f32> &colour, StringView title, u32 font_id, const gouda::Colour<f32> &title_colour,
              f32 title_scale, PanelSide side = PanelSide::Right,
              gouda::math::easing::EasingType easing_type = gouda::math::easing::EasingType::Linear);

    void ToggleVisibility();
    void SetSide(PanelSide side);
    void ToggleSide();

    void SetEasingType(const gouda::math::easing::EasingType type) {
        m_easing_type = type;
    }

    void Update(f32 delta_time);
    void Draw(std::vector<gouda::InstanceData> &quad_instances, std::vector<gouda::TextData> &text_instances) const;

    void OnFramebufferResize(const gouda::Vec2 &new_size);

private:
    void UpdatePosition(f32 delta_time);

private:
    SharedContext &m_shared_context;
    gouda::InstanceData m_instance_data;
    PanelSide m_panel_side;
    gouda::math::easing::EasingType m_easing_type;

    // TODO: Set this in the constructor
    f32 m_transition_time{0.5f}; // total time for open/close
    f32 m_animation_timer{0.0f}; // tracks time in animation
    bool m_is_open{false};       // target state
    bool m_is_animating{false};  // whether animation is in progress

    gouda::Vec2 m_size; // fixed panel size
    gouda::Vec2 m_padding;
    gouda::Vec2 m_screen_size; // needed for positioning

    String m_title;
    u32 m_font_id;
    gouda::Colour<f32> m_text_colour;
    gouda::Vec3 m_title_position;
    f32 m_text_scale;
};
