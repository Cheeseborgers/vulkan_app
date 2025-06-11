/**
 * @file side_panel.cpp
 * @author GoudaCheeseburgers
 * @date 2025-11-06
 * @brief Application side panel module implementation
 */
#include "ui/side_panel.hpp"

#include "math/easing.hpp"

SidePanel::SidePanel(SharedContext &shared_context, const gouda::Vec2 &size, const gouda::Vec2 &padding,
                     const gouda::Colour<f32> &colour, StringView title, const u32 font_id,
                     const gouda::Colour<f32> &title_colour, const f32 title_scale, const PanelSide side,
                     const gouda::math::easing::EasingType easing_type)
    : m_shared_context{shared_context},
      m_panel_side{side},
      m_easing_type{easing_type},
      m_size{size},
      m_padding{padding},
      m_font_id{font_id}
{
    m_screen_size = {static_cast<f32>(m_shared_context.renderer->GetFramebufferSize().width),
                     static_cast<f32>(m_shared_context.renderer->GetFramebufferSize().height)};

    m_size = {m_screen_size.x * size.x, m_screen_size.y * size.y};
    m_padding = padding;

    m_instance_data.size = m_size;
    m_instance_data.position = {m_screen_size.x, 0.0f, -0.99f}; // Start off-screen
    m_instance_data.texture_index = 0;
    m_instance_data.colour = colour;
    m_instance_data.apply_camera_effects = 0;

    m_text_colour = title_colour;
    m_text_scale = title_scale;
    m_title = title;

    m_title_position.x = m_instance_data.position.x + m_padding.x;
    m_title_position.y = m_size.y - m_text_scale - m_padding.y;
    m_title_position.z = m_instance_data.position.z - -0.01f;
}

void SidePanel::Toggle()
{
    m_is_open = !m_is_open;
    m_is_animating = true;
    m_animation_timer = 0.0f;
}

void SidePanel::SetSide(const PanelSide side)
{
    if (m_panel_side != side) {
        m_panel_side = side;

        // Snap to the correct closed/open position immediately
        m_is_animating = false;
        m_animation_timer = 0.0f;
        UpdatePosition(1.0f); // Force update to final position
    }
}

void SidePanel::ToggleSide() { SetSide(m_panel_side == PanelSide::Left ? PanelSide::Right : PanelSide::Left); }

void SidePanel::Update(const f32 delta_time)
{
    if (!m_is_animating) {
        return;
    }

    m_animation_timer += delta_time;

    f32 t = gouda::math::min(m_animation_timer / m_transition_time, 1.0f);
    t = gouda::math::easing::ApplyEasing(t, m_easing_type);

    UpdatePosition(t);

    if (m_animation_timer >= m_transition_time) {
        m_is_animating = false;
        UpdatePosition(1.0f); // Snap to final position
    }
}

void SidePanel::Render(std::vector<gouda::InstanceData> &quad_instances,
                       std::vector<gouda::TextData> &text_instances) const
{
    if (m_is_open || m_is_animating) {
        quad_instances.push_back(m_instance_data);
        m_shared_context.renderer->DrawText(m_title, m_title_position, m_text_colour, m_text_scale, m_font_id,
                                            text_instances, gouda::TextAlign::Left, false);
    }
}

void SidePanel::OnFramebufferResize(const gouda::Vec2 &new_size)
{
    m_screen_size = new_size;

    m_size = {m_screen_size.x * m_size.x, m_screen_size.y * m_size.y};

    m_instance_data.size = m_size;
    m_instance_data.position = {m_screen_size.x, 0.0f, -0.99f}; // Start off-screen

    m_title_position.x = m_instance_data.position.x + m_padding.x;
    m_title_position.y = m_size.y - m_text_scale - m_padding.y;
    m_title_position.z = m_instance_data.position.z - -0.01f;
}

void SidePanel::UpdatePosition(const f32 delta_time)
{
    f32 closed_x{0.0f};
    f32 open_x{0.0f};

    if (m_panel_side == PanelSide::Right) {
        closed_x = m_screen_size.x;          // Off-screen right
        open_x = m_screen_size.x - m_size.x; // On-screen right
    }
    else {
        closed_x = -m_size.x; // Off-screen left
        open_x = 0.0f;        // On-screen left
    }

    const f32 new_x{m_is_open ? gouda::math::lerp(closed_x, open_x, delta_time)
                              : gouda::math::lerp(open_x, closed_x, delta_time)};

    m_instance_data.position.x = new_x;
    m_title_position.x = m_instance_data.position.x + m_padding.x;
}
