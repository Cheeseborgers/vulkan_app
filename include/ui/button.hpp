#pragma once
/**
 * @file ui/button.hpp
 * @author GoudaCheeseburgers
 * @date 2025-08-06
 * @brief Application ui module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "ui_element.hpp"

struct Button final : UIElement {
    Button(const gouda::Vec3 &position, const gouda::Vec2 &size, const f32 rotation, const u32 texture_index,
           const gouda::Colour<f32> &tint_colour, const UVRect<f32> &sprite_rect, const u32 is_atlas)
        : UIElement(
              gouda::InstanceData{position, size, rotation, texture_index, tint_colour, sprite_rect, is_atlas, 0}),
          font_id{1}
    {
    }

    void Render(std::vector<gouda::InstanceData>& quad_instances, std::vector<gouda::TextData>& text_instances) const override {

        if (!text.empty()) {
            gouda::Vec3 text_position{
                instance_data.position.x + instance_data.size.x / 2.f,
                instance_data.position.y + instance_data.size.y / 2.f,
                instance_data.position.z - 0.001f // Slightly above quad
            };
        }
    }

    bool HandleInput(const gouda::InputHandler& input, std::function<void()>& callback) override {
        if (input.IsMouseButtonPressed(gouda::MouseButton::Left)) {

        }
        return false;
    }

    String text;
    u16 font_id;

    CallbackFunction<> callback;
};
