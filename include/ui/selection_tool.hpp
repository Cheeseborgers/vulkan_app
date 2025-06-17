#pragma once
/**
 * @file ui/selection_tool.hpp
 * @author GoudaCheeseburgers
 * @date 2025-16-06
 * @brief Application selection tool module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "math/math.hpp"
#include "renderers/render_data.hpp"
#include "containers/small_vector.hpp"

struct Entity;

class SelectionTool {
public:
    SelectionTool();

    void BeginSelection(const gouda::Vec2 &start);
    void UpdateSelection(const gouda::Vec2 &end);
    void EndSelection(const std::vector<Entity*>& all_entities);

    void Draw(gouda::Vector<gouda::InstanceData>& quad_instances);

    void Reset();

    [[nodiscard]] bool IsSelecting() const;

private:
    gouda::InstanceData m_selection_box_instance;

    gouda::Vec2 m_start;
    gouda::Vec2 m_end;
    gouda::Vector<Entity *> m_selected;

    bool m_selecting;
};

