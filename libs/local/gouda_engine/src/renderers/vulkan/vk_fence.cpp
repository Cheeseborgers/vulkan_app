#include "renderers/vulkan/vk_fence.hpp"

#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_utils.hpp"

namespace gouda::vk {

Fence::Fence(Device *device) : p_fence(VK_NULL_HANDLE), p_device(device) {}

Fence::~Fence()
{
    if (p_fence != VK_NULL_HANDLE && p_device != nullptr) {
        vkDestroyFence(p_device->GetDevice(), p_fence, nullptr); // Destroy the fence
        p_fence = VK_NULL_HANDLE;                                // Ensure the fence handle is invalidated
        ENGINE_LOG_DEBUG("Fence destroyed");
    }
}

Fence::Fence(Fence &&other) noexcept : p_fence(other.p_fence), p_device(other.p_device)
{
    other.p_fence = VK_NULL_HANDLE; // Nullify the other object's fence handle
}

Fence &Fence::operator=(Fence &&other) noexcept
{
    if (this != &other) { // Avoid self-assignment
        // Clean up the current object
        if (p_fence != VK_NULL_HANDLE && p_device != nullptr) {
            vkDestroyFence(p_device->GetDevice(), p_fence, nullptr);
        }

        // Transfer ownership from other
        p_fence = other.p_fence;
        p_device = other.p_device;
        other.p_fence = VK_NULL_HANDLE; // Nullify the other object's fence handle
    }
    return *this;
}

bool Fence::Create(const VkFenceCreateFlags flags)
{
    return CreateFence(flags); // Call the helper function to create the fence
}

bool Fence::CreateFence(const VkFenceCreateFlags flags)
{
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = flags; // Set the desired flags for the fence

    const VkResult result = vkCreateFence(p_device->GetDevice(), &fence_info, nullptr, &p_fence);
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateFence");
    }

    return result == VK_SUCCESS;
}

void Fence::WaitFor(const u64 timeout) const
{
    if (const VkResult result = vkWaitForFences(p_device->GetDevice(), 1, &p_fence, VK_TRUE, timeout);
        result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkWaitForFences");
    }
}

void Fence::Reset() const
{
    if (const VkResult result = vkResetFences(p_device->GetDevice(), 1, &p_fence); result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkResetFences");
    }
}

bool Fence::IsSignaled() const
{
    const VkResult result = vkGetFenceStatus(p_device->GetDevice(), p_fence);
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkGetFenceStatus");
    }

    return result == VK_SUCCESS; // Return true if the fence is signaled
}

} // namespace gouda::vk
