/**
 * @file ui_element.cpp
 * @author GoudaCheeseburgers
 * @date 2025-08-06
 * @brief Application ui element module implementation
 */
#include "ui/ui_element.hpp"

UIElement::UIElement(const gouda::InstanceData &instance_data)
    : instance_data{instance_data}, dirty{true}, parent{nullptr}
{
}

void UIElement::SetPosition(const gouda::Vec3 &position)
{
    instance_data.position = position;
    dirty = true;
}
void UIElement::SetSize(const gouda::Vec2 &size)
{
    instance_data.size = size;
    dirty = true;
}
void UIElement::AddChild(std::unique_ptr<UIElement> child)
{
    child->parent = this;
    children.push_back(std::move(child));
}