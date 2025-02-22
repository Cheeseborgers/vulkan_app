#pragma once

#include "gouda_types.hpp"

#include "gouda_vk_core.hpp"
#include "gouda_vk_glfw.hpp"
#include "gouda_vk_graphics_pipeline.hpp"
#include "gouda_vk_shader.hpp"
#include "gouda_vk_simple_mesh.hpp"
#include "gouda_vk_utils.hpp"

namespace GoudaVK {

void BeginCommandBuffer(VkCommandBuffer CommandBuffer, VkCommandBufferUsageFlags UsageFlags);

void CreateSemaphore(VkDevice device, VkSemaphore &semaphore);

} // end namespace