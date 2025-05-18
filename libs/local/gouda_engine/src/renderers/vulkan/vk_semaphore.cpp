/**
 * @file vk_semaphore.cpp
 * @author GoudaCheeseburgers
 * @date 2025-04-24
 * @brief Engine module implementation
 */
#include "renderers/vulkan/vk_semaphore.hpp"

#include "debug/logger.hpp"

#include "debug/assert.hpp"
#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_utils.hpp"

namespace gouda::vk {

Semaphore::Semaphore(Device *device) : p_semaphore{VK_NULL_HANDLE}, p_device{device}
{
    ASSERT(device, "Device cannot be a null pointer when creating a semaphore.");
}

Semaphore::~Semaphore() { Destroy(); }

bool Semaphore::Create()
{
    if (!p_device) {
        ENGINE_LOG_ERROR("Cannot create a semaphore with an invalid/null device pointer.");
        return false;
    }

    if (p_semaphore != VK_NULL_HANDLE) {
        ENGINE_LOG_ERROR("Semaphore has already been created.");
        return false;
    }

    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (const VkResult result{vkCreateSemaphore(p_device->GetDevice(), &semaphore_create_info, nullptr, &p_semaphore)};
        result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateSemaphore");
        return false;
    }

    return true;
}

void Semaphore::Destroy()
{
    if (p_device && p_semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(p_device->GetDevice(), p_semaphore, nullptr);
        p_semaphore = VK_NULL_HANDLE;

        ENGINE_LOG_DEBUG("Semaphore destroyed.");
    }
}

} // namespace gouda::vk