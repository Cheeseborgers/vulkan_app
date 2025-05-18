#pragma once
/**
 * @file vk_queue.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-24
 * @brief Engine module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <vulkan/vulkan.h>

#include "containers/small_vector.hpp"
#include "core/types.hpp"

namespace gouda::vk {

class Device;
class Fence;
class Swapchain;

class Queue {

public:
    Queue();
    ~Queue();

    void Initialize(Device *device, Swapchain *swapchain, u32 queue_family, u32 queue_index);
    void Destroy();

    [[nodiscard]] u32 AcquireNextImage(u32 frame_index);

    void Submit(VkCommandBuffer command_buffer, u32 frame_index, const Fence *fence = nullptr); // For render loop
    void Submit(VkCommandBuffer command_buffer, const Fence *fence = nullptr) const;                  // For standalone ops
    void SetSwapchain(Swapchain *swapchain) { p_swapchain = swapchain; }
    void Present(u32 image_index, u32 frame_index);

    void WaitIdle() const;

    [[nodiscard]] static u32 GetMaxFramesInFlight() noexcept { return MAX_FRAMES_IN_FLIGHT; }

private:
    void CreateSemaphores();

private:
    Device *p_device;
    Swapchain *p_swapchain;
    VkQueue p_queue;

    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 4;
    SmallVector<VkSemaphore, MAX_FRAMES_IN_FLIGHT> p_present_complete_semaphores;
    SmallVector<VkSemaphore, MAX_FRAMES_IN_FLIGHT> p_render_complete_semaphores;
};

} // namespace gouda::vk