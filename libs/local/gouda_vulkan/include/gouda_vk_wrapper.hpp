#pragma once

#include "gouda_types.hpp"

#include "gouda_vk_core.hpp"
#include "gouda_vk_graphics_pipeline.hpp"
#include "gouda_vk_shader.hpp"
#include "gouda_vk_simple_mesh.hpp"
#include "gouda_vk_utils.hpp"

namespace GoudaVK {

void BeginCommandBuffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage_flags);

void EndCommandBuffer(VkCommandBuffer command_buffer);

void CreateSemaphore(VkDevice device, VkSemaphore &semaphore);

void CreateFence(VkDevice device, const VkFenceCreateInfo *create_info_ptr, const VkAllocationCallbacks *allocator_ptr,
                 VkFence *fence_ptr);

} // end namespace