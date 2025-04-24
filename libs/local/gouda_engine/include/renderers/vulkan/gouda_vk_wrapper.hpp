#pragma once

#include "core/types.hpp"

#include "vk_graphics_pipeline.hpp"
#include "vk_renderer.hpp"
#include "vk_shader.hpp"
#include "vk_utils.hpp"

namespace gouda {
namespace vk {

void BeginCommandBuffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage_flags);

void EndCommandBuffer(VkCommandBuffer command_buffer);

void CreateSemaphore(VkDevice device, VkSemaphore &semaphore);

void CreateFence(VkDevice device, const VkFenceCreateInfo *create_info_ptr, const VkAllocationCallbacks *allocator_ptr,
                 VkFence *fence_ptr);

} // namesapce vk
} // namespace gouda