/**
* @file selection_tool.cpp
 * @author GoudaCheeseburgers
 * @date 2025-16-06
 * @brief Application selection tool module implementation
 */
#include "ui/selection_tool.hpp"

#include "entities/entity.hpp"

SelectionTool::SelectionTool() : m_selecting{false} { Reset(); }

void SelectionTool::BeginSelection(const gouda::Vec2 &start)
{
    m_start = start;
    m_end = start;
    m_selecting = true;
    m_selected.clear();
}
void SelectionTool::UpdateSelection(const gouda::Vec2 &end)
{
    m_end = end;
}

void SelectionTool::EndSelection(const std::vector<Entity *> &all_entities)
{
    m_selecting = false;
    m_selected.clear();

    const f32 min_x = gouda::math::min(m_start.x, m_end.x);
    const f32 max_x = gouda::math::max(m_start.x, m_end.x);
    const f32 min_y = gouda::math::min(m_start.y, m_end.y);
    const f32 max_y = gouda::math::max(m_start.y, m_end.y);
    [[maybe_unused]] Rect<f32> selection_rect{min_x, max_x, min_y, max_y}; // TODO: Rename members of Rect<T>

    for ([[maybe_unused]] auto & entity : all_entities) {
        //if (entity->GetBounds().Intersects(selection_rect)) {
         //   m_selected.push_back(entity);
       // }
    }
}
void SelectionTool::Draw(gouda::Vector<gouda::InstanceData> &quad_instances)
{
    if (!m_selecting) {
        return;
    }

    const gouda::Vec2 size{gouda::math::abs(m_end.x - m_start.x), gouda::math::abs(m_end.y - m_start.y)};
    const gouda::Vec2 position{gouda::math::min(m_start.x, m_end.x), gouda::math::min(m_start.y, m_end.y)};

    m_selection_box_instance.position = {position.x, position.y, m_selection_box_instance.position.y};
    m_selection_box_instance.size = size;

    quad_instances.push_back(m_selection_box_instance);
}

void SelectionTool::Reset()
{
    m_selecting = false;
    m_selected.clear();

    // TODO: FIX the z layer for positioning
    m_selection_box_instance.position = {0.0f, 0.0f, -0.1f};
    m_selection_box_instance.size = {0.0f, 0.0f};
    m_selection_box_instance.colour = {0.2f, 0.7f, 0.2f, 0.3f};
    m_selection_box_instance.texture_index = 0;
    m_selection_box_instance.apply_camera_effects = false;

    m_start = {0.0f, 0.0f};
    m_end = {0.0f, 0.0f};
}
bool SelectionTool::IsSelecting() const
{
    return m_selecting;
}
