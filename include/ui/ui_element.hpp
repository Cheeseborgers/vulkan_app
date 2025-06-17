#pragma once
/**
 * @file ui/ui_element.hpp
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
#include "backends/input_handler.hpp"
#include "containers/small_vector.hpp"
#include "renderers/render_data.hpp"

enum class UIElementState : u8 { HOVERED, ACTIVE, NONE };

struct UIElement {
    explicit UIElement(const gouda::InstanceData &instance_data);
    virtual ~UIElement() = default;

    virtual void Update(f32 delta_time) = 0;
    virtual void Draw(std::vector<gouda::InstanceData> &quad_instances,
                        std::vector<gouda::TextData> &text_instances) = 0;
    virtual bool HandleInput(const gouda::InputHandler &input, std::function<void()> &callback) = 0;

    void SetPosition(const gouda::Vec3 &position);
    void SetSize(const gouda::Vec2 &size);
    void AddChild(std::unique_ptr<UIElement> child);

    gouda::InstanceData instance_data;
    bool dirty;
    UIElement *parent;
    gouda::Vector<std::unique_ptr<UIElement>> children;
};
