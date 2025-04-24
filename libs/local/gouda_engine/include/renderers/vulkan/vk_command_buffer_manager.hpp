#pragma once
/**
 * @file vk_command_buffer_manager.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-19
 * @brief Engine module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <vulkan/vulkan.h>

#include "core/types.hpp"

namespace gouda::vk {

class Device;
class Queue;

class CommandBufferManager {
public:
    CommandBufferManager(Device *device, Queue *queue, u32 queue_family);
    ~CommandBufferManager();

    void CreatePool();
    void AllocateBuffers(u32 count, VkCommandBuffer *buffers);
    void FreeBuffers(u32 count, const VkCommandBuffer *buffers);

private:
    Device *p_device;
    Queue *p_queue;
    u32 m_queue_family;
    VkCommandPool p_pool;
};

} // namespace gouda::vk