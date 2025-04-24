#include "renderers/vulkan/vk_fence.hpp"

#include "renderers/vulkan/vk_device.hpp" // Assuming Device class is defined here

namespace gouda::vk {

// Constructor: Initializes the Fence object with the device pointer
Fence::Fence(Device *device) : p_device(device), p_fence(VK_NULL_HANDLE) {}

// Destructor: Cleans up the Vulkan fence when the object is destroyed
Fence::~Fence()
{
    if (p_fence != VK_NULL_HANDLE && p_device != nullptr) {
        vkDestroyFence(p_device->GetDevice(), p_fence, nullptr); // Destroy the fence
        p_fence = VK_NULL_HANDLE;                                // Ensure the fence handle is invalidated
        ENGINE_LOG_DEBUG("Fence destroyed");
    }
}

// Move constructor: Transfers ownership of the fence from another object
Fence::Fence(Fence &&other) noexcept : p_device(other.p_device), p_fence(other.p_fence)
{
    other.p_fence = VK_NULL_HANDLE; // Nullify the other object's fence handle
}

// Move assignment operator: Transfers ownership of the fence from another object
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

// Create the Vulkan fence with the specified flags
bool Fence::Create(VkFenceCreateFlags flags)
{
    return CreateFence(flags); // Call the helper function to create the fence
}

// Helper function to create the Vulkan fence
bool Fence::CreateFence(VkFenceCreateFlags flags)
{
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = flags; // Set the desired flags for the fence

    VkResult result = vkCreateFence(p_device->GetDevice(), &fence_info, nullptr, &p_fence);
    return result == VK_SUCCESS; // Return true if creation was successful
}

// Wait for the fence to be signaled, with an optional timeout
bool Fence::WaitFor(uint64_t timeout)
{
    VkResult result = vkWaitForFences(p_device->GetDevice(), 1, &p_fence, VK_TRUE, timeout);
    return result == VK_SUCCESS; // Return true if the fence was signaled
}

// Reset the fence, allowing it to be reused
bool Fence::Reset()
{
    VkResult result = vkResetFences(p_device->GetDevice(), 1, &p_fence);
    return result == VK_SUCCESS; // Return true if reset was successful
}

// Check if the fence is signaled
bool Fence::IsSignaled() const
{
    VkResult result = vkGetFenceStatus(p_device->GetDevice(), p_fence);
    return result == VK_SUCCESS; // Return true if the fence is signaled
}

} // namespace gouda::vk
