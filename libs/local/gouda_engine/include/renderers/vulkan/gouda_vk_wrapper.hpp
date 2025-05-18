#pragma once

#include "vk_shader.hpp"


namespace gouda::vk {

void BeginCommandBuffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage_flags);

void EndCommandBuffer(VkCommandBuffer command_buffer);

void CreateSemaphore(VkDevice device, VkSemaphore &semaphore);

}