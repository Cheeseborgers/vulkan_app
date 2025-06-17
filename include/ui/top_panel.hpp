#pragma once
/**
 * @file ui/top_panel.hpp
 * @author GoudaCheeseburgers
 * @date 2025-16-06
 * @brief Application top panel menu module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "core/state_stack.hpp"
#include "renderers/render_data.hpp"

struct TopPanelButton {
    TopPanelButton() = default;

    void Update(f32 delta_time)
    {
        // Handle onclick etc... as buttons may handle a menu??
    }

    void Render(gouda::vk::Renderer &renderer, std::vector<gouda::InstanceData> &quad_instances,
                std::vector<gouda::TextData> &text_instances)
    {
        quad_instances.emplace_back(instance);

        renderer.DrawText(button_text, {}, colours::editor_panel_primary_font_colour, 20.0f, 1, text_instances);
    }

    gouda::InstanceData instance;
    String button_text;
    gouda::Vec2 padding;
};

// TODO: Add auto hide using easing

class TopPanel {
public:
    explicit TopPanel(SharedContext &context, const f32 height, const gouda::Colour<f32> &colour,
                      const gouda::Vec2 &padding, const u32 font_id, const f32 font_scale,
                      const gouda::Colour<f32> &font_colour)
        : m_context{context},
          m_padding{padding},
          m_font_id{font_id},
          m_font_scale{font_scale},
          m_font_colour{font_colour},
          m_transition_time{0.5f},
          m_animation_timer{0.0f},
          m_is_open{true},
          m_is_animating{false},
          m_auto_hide{false}
    {
        m_window_size = {static_cast<f32>(context.window->GetWindowSize().width),
                         static_cast<f32>(context.window->GetWindowSize().height)};

        m_instance.position = {0.0f, m_window_size.y - height, -0.9f};
        m_instance.size = {m_window_size.x, height};
        m_instance.colour = colour;
        m_instance.apply_camera_effects = false;
    }

    void AddButton(const TopPanelButton &button) { m_buttons.emplace_back(button); }

    void Toggle()
    {
        m_is_open = !m_is_open;
        m_is_animating = true;
        m_animation_timer = 0.0f;
    }

    void Open()
    {
        if (!m_is_open) {
            Toggle();
        }
    }

    void Close()
    {
        if (m_is_open) {
            Toggle();
        }
    }

    void EnableAutoHide(const bool enabled) { m_auto_hide = enabled; }

    void HandleInput(const gouda::Vec2 &mouse_position)
    {

        const bool is_hovered = mouse_position.y >= m_instance.position.y;

        if (m_auto_hide && !m_is_animating) {
            if (!m_is_open && is_hovered) {
                Toggle(); // Slide down
            }
            else if (m_is_open && !is_hovered) {
                Toggle(); // Slide up
            }
        }
    }

    void Update(const f32 delta_time)
    {
        if (!m_is_animating) {
            return;
        }

        m_animation_timer += delta_time;

        f32 t = gouda::math::min(m_animation_timer / m_transition_time, 1.0f);
        t = gouda::math::easing::ApplyEasing(t, gouda::math::easing::EasingType::EaseInOutCubic);

        UpdatePosition(t);

        if (m_animation_timer >= m_transition_time) {
            m_is_animating = false;
            UpdatePosition(1.0f); // Snap to final position
        }
    }

    void Draw(std::vector<gouda::InstanceData> &quad_instances, std::vector<gouda::TextData> &text_instances)
    {
        if (!m_is_open) {
            return;
        }

        // Draw the panel
        quad_instances.emplace_back(m_instance);

        for (auto &button : m_buttons) {
            button.Render(*m_context.renderer, quad_instances, text_instances);
        }
    }

    [[nodiscard]] gouda::Vec2 GetSize() const { return m_instance.size; }

private:
    void UpdatePosition(const f32 t)
    {
        const f32 open_y = m_window_size.y - m_instance.size.y;
        const f32 closed_y = m_window_size.y;

        // Lerp between closed and open
        const f32 target_y =
            m_is_open ? gouda::math::lerp(closed_y, open_y, t) : gouda::math::lerp(open_y, closed_y, t);

        m_instance.position.y = target_y;
    }

private:
    SharedContext &m_context;
    gouda::InstanceData m_instance;

    gouda::Vector<TopPanelButton> m_buttons;
    gouda::Vec2 m_window_size;
    gouda::Vec2 m_padding;

    u32 m_font_id;
    f32 m_font_scale;
    gouda::Colour<f32> m_font_colour;

    f32 m_transition_time; // Total time for open/close
    f32 m_animation_timer; // Tracks time in animation
    bool m_is_open;        // Target state
    bool m_is_animating;   // Whether animation is in progress
    bool m_auto_hide;      // Auto hide until hovered
};
