/**
 * @file ui_manager.cpp
 * @author GoudaCheeseburgers
 * @date 2025-08-06
 * @brief Application ui manager module implementation
 */
#include "ui/ui_manager.hpp"

UIManager::UIManager(SharedContext &context) : m_context{context} {}

void UIManager::AddElement(std::unique_ptr<UIElement> element) { m_elements.push_back(std::move(element)); }

void UIManager::Update(const f32 delta_time)
{
    for (const auto &element : m_elements) {
        element->Update(delta_time);
    }
}
void UIManager::Draw(gouda::vk::Renderer &renderer, std::vector<gouda::InstanceData> &quad_instances,
                     std::vector<gouda::TextData> &text_instances)
{
    for (const auto &element : m_elements) {
     element->Draw(quad_instances, text_instances);
    }
}

void UIManager::HandleInput()
{
    for (const auto &element : m_elements) {
        std::function<void()> callback;
        if (element->HandleInput(*m_context.input_handler, callback)) {
            callback();
        }
    }
}
