/**
 * @file editor_popups.cpp
 * @author GoudaCheeseburgers
 * @date 2025-17-06
 * @brief Application selection tool module implementation
 */
#include "ui/editor_popups.hpp"

// Editor popup implementation  -----------------------------------------------------------------
EditorPopUp::EditorPopUp(SharedContext &shared_context, const gouda::Vec3 &position, const gouda::Vec2 &size,
                         const gouda::Colour<f32> &colour, const u32 texture_index, const gouda::Vec2 &padding,
                         const u32 font_id, const f32 font_scale, const gouda::Colour<f32> &text_colour)
    : shared_context{shared_context},
      padding{padding},
      font_id{font_id},
      font_scale{font_scale},
      font_colour{text_colour}
{
    instance.position = position;
    instance.size = size;
    instance.colour = colour;
    instance.texture_index = texture_index;

    instance.is_atlas = false;
    instance.apply_camera_effects = false;
}

void EditorPopUp::SetPosition(const gouda::Vec3 &position) { instance.position = position; }
void EditorPopUp::SetTexture(const u32 texture_index) { instance.texture_index = texture_index; }
void EditorPopUp::SetColour(const gouda::Colour<f32> &colour) { instance.colour = colour; }
void EditorPopUp::SetPadding(const gouda::Vec2 &padding_value) { padding = padding_value; }
void EditorPopUp::SetFontColour(const gouda::Colour<f32> &colour) { font_colour = colour; }
void EditorPopUp::SetFontScale(const f32 scale) { font_scale = scale; }
void EditorPopUp::SetFontId(const u32 new_id) { font_id = new_id; }

// Exit confirmation popup implementation  -----------------------------------------------------------------
ExitConfirmationPopup::ExitConfirmationPopup(SharedContext &shared_context, const gouda::Vec3 &position,
                                             const gouda::Vec2 &size, const gouda::Colour<f32> &colour,
                                             const u32 texture_index, const gouda::Vec2 &padding, const u32 font_id,
                                             const f32 font_scale, const gouda::Colour<f32> &font_colour)
    : EditorPopUp(shared_context, position, size, colour, texture_index, padding, font_id, font_scale, font_colour)
{
    // TODO: Setup buttons
}

void ExitConfirmationPopup::Draw(std::vector<gouda::InstanceData> &quad_instances,
                                 std::vector<gouda::TextData> &text_instances)
{
    quad_instances.emplace_back(instance);

    // TODO: Draw text

    // TODO: Draw buttons
}

// Entity popup implementation  -----------------------------------------------------------------
EntityPopup::EntityPopup(SharedContext &shared_context, Entity *entity, const gouda::Vec2 &size,
                         const gouda::Colour<f32> &colour, const u32 texture_index, const gouda::Vec2 &padding,
                         const u32 font_id, const f32 font_scale, const gouda::Colour<f32> &font_colour)
    : EditorPopUp(shared_context, {0.0f}, size, colour, texture_index, padding, font_id, font_scale, font_colour),
      entity{entity}
{
    // TODO: Fix rendering z layering for good!!

    // Set the position based on the selected entity position
    instance.position = {entity->render_data.position.x + 10.0f, entity->render_data.position.y + 10.0f, -0.111111f};
}

void EntityPopup::HandleInput() {}

void EntityPopup::Update(f32 delta_time) {}

void EntityPopup::Draw(std::vector<gouda::InstanceData> &quad_instances, std::vector<gouda::TextData> &text_instances)
{
    if (!entity) {
        APP_LOG_INFO("entity is null");
        return;
    }

    quad_instances.emplace_back(instance);

    f32 current_position_y{instance.position.y + instance.size.y};

    // TODO: Sort text z laying
    APP_LOG_INFO("Positon: {}", entity->render_data.position.ToString());
    gouda::Vector<String> lines = {std::format("POSITION 9 {:.2f}:{:.2f}:{:.2f}", entity->render_data.position.x,
                                               entity->render_data.position.y, entity->render_data.position.z),
                                   std::format("TEXTURE {}", entity->render_data.texture_index), "SOME MORE TEXT 9",
                                   "SOME MORE TEXT"};

    for (const auto &line : lines) {
        current_position_y -= font_scale + padding.y;
        const gouda::Vec3 position{instance.position.x + padding.x, current_position_y - padding.y, -0.1f};
        shared_context.renderer->DrawText(line, position, font_colour, font_scale, font_id, text_instances);
    }
}
