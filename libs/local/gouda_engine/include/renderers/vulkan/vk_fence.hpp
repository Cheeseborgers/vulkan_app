#pragma once

/**
 * @file vk_fence.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-24
 * @brief Engine module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "core/types.hpp"
#include <vulkan/vulkan.h>

namespace gouda::vk {

class Device; // Forward declaration of the Device class

class Fence {
public:
    // Constructor that accepts a device pointer to initialize the fence
    explicit Fence(Device *device);

    // Destructor cleans up the Vulkan fence when the object is destroyed
    ~Fence();

    // Move constructor (transfers ownership of the fence from another object)
    Fence(Fence &&other) noexcept;

    // Move assignment operator (transfers ownership of the fence from another object)
    Fence &operator=(Fence &&other) noexcept;

    // Deleted copy constructor and assignment operator to avoid copying
    Fence(const Fence &) = delete;
    Fence &operator=(const Fence &) = delete;

    // Creates a Vulkan fence and returns true if successful
    [[nodiscard]] bool Create(VkFenceCreateFlags flags = 0);

    // Waits for the fence to be signaled with an optional timeout
    void WaitFor(uint64_t timeout = UINT64_MAX) const;

    // Resets the fence, allowing it to be reused
    void Reset() const;

    // Checks if the fence is signaled
    [[nodiscard]] bool IsSignaled() const;

    // Returns the raw Vulkan fence handle (to be used with Vulkan API calls)
    [[nodiscard]] VkFence Get() const noexcept { return p_fence; }

private:
    // Helper function to create the Vulkan fence
    bool CreateFence(VkFenceCreateFlags flags);

private:
    VkFence p_fence;  // Vulkan fence handle
    Device *p_device; // Pointer to the associated Vulkan device
};

} // namespace gouda::vk
