#pragma once
/**
 * @file ui/editor_popups.hpp
 * @author GoudaCheeseburgers
 * @date 2025-17-06
 * @brief Application ui editor popups module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "core/types.hpp"

#include "core/state_stack.hpp"
#include "debug/logger.hpp"
#include "entities/entity.hpp"

class EditorPopUp {
public:
    EditorPopUp(SharedContext &shared_context, const gouda::Vec3 &position, const gouda::Vec2 &size,
                const gouda::Colour<f32> &colour, u32 texture_index, const gouda::Vec2 &padding,
                u32 font_id, f32 font_scale, const gouda::Colour<f32> &text_colour);

    void SetPosition(const gouda::Vec3 &position);
    void SetTexture(u32 texture_index);
    void SetColour(const gouda::Colour<f32> &colour);
    void SetPadding(const gouda::Vec2 &padding_value);
    void SetFontColour(const gouda::Colour<f32> &colour);
    void SetFontScale(f32 scale);
    void SetFontId(u32 new_id);

protected:
    SharedContext &shared_context;
    gouda::InstanceData instance;

    gouda::Vec2 padding;

    u32 font_id;
    f32 font_scale;
    gouda::Colour<f32> font_colour;
};

struct ExitConfirmationPopup : EditorPopUp {
    ExitConfirmationPopup(SharedContext &shared_context, const gouda::Vec3 &position, const gouda::Vec2 &size,
                          const gouda::Colour<f32> &colour, u32 texture_index, const gouda::Vec2 &padding,
                          u32 font_id, f32 font_scale, const gouda::Colour<f32> &font_colour);

    void Draw(std::vector<gouda::InstanceData> &quad_instances, std::vector<gouda::TextData> &text_instances);
};

class EntityPopup : EditorPopUp {
public:
    EntityPopup(SharedContext &shared_context, Entity *entity, const gouda::Vec2 &size,
                const gouda::Colour<f32> &colour, u32 texture_index, const gouda::Vec2 &padding,
                u32 font_id, f32 font_scale, const gouda::Colour<f32> &font_colour);

    void HandleInput();
    void Update(f32 delta_time);
    void Draw(std::vector<gouda::InstanceData> &quad_instances, std::vector<gouda::TextData> &text_instances);

private:
    Entity *entity;
};
