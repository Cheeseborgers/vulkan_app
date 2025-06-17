#pragma once
/**
 * @file ui/ui_manager.hpp
 * @author GoudaCheeseburgers
 * @date 2025-08-06
 * @brief Application ui manager module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "containers/small_vector.hpp"

#include "core/state_stack.hpp"
#include "ui_element.hpp"

class UIManager {
public:
    explicit UIManager(SharedContext &context);
    ~UIManager() = default;

    void AddElement(std::unique_ptr<UIElement> element);

    void Update(f32 delta_time);
    void Draw(gouda::vk::Renderer &renderer, std::vector<gouda::InstanceData> &quad_instances,
                std::vector<gouda::TextData> &text_instances);
    void HandleInput();

private:
    SharedContext &m_context;
    gouda::Vector<std::unique_ptr<UIElement>> m_elements;
};