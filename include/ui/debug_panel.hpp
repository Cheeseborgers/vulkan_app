#pragma once
/**
 * @file ui/debug_panel.hpp
 * @author GoudaCheeseburgers
 * @date 2025-16-06
 * @brief Application debug panel module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "core/constants.hpp"
#include "core/state_stack.hpp"
#include "renderers/render_data.hpp"

struct DebugPanel {
    DebugPanel(SharedContext &shared_context, const gouda::Vec2 &size,
               const gouda::Colour<f32> &colour, const u32 font_id, const f32 font_scale)
        : context{shared_context}, font_id{font_id}, font_scale{font_scale}, padding{5.0f}, display{false}
    {
        instance.size = size;

        instance.position.x = 10.0f;
        instance.position.y = static_cast<f32>(context.window->GetWindowSize().height) - size.y;
        instance.position.z = -0.9f;

        instance.colour = colour;
        instance.texture_index = 0;
        instance.apply_camera_effects = false;
    }

    void Render(std::vector<gouda::InstanceData> &quad_instances,
                        std::vector<gouda::TextData> &text_instances)
    {
        if (!display) {
            return;
        }

        // Draw the panel
        quad_instances.emplace_back(instance);

        // Draw the text
        //gouda::vk::RenderStatistics render_statistics = context.renderer->GetRenderStatistics();

        f32 current_position_y{instance.position.y + instance.size.y};
        gouda::Vector<String> lines = {"HELLO", "WORLD", "PIPE"};

        for (const auto &line : lines) {
            current_position_y -= font_scale;
            const gouda::Vec3 position{instance.position.x + padding.x, current_position_y, -0.1f};
            context.renderer->DrawText(line, position, colours::editor_panel_primary_font_colour, font_scale,
                                              font_id, text_instances);
        }
    }


    void Toggle() { display = !display; }

    SharedContext &context;
    gouda::InstanceData instance;

    u32 font_id;
    f32 font_scale;
    gouda::Vec2 padding;
    gouda::Colour<f32> text_colour;
    bool display;
};
