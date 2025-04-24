#pragma once
/**
 * @file gouda_vk_buffer.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-15
 * @brief Engine module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <vulkan/vulkan.h>

namespace gouda::vk {

struct Buffer {
    Buffer();

    void *MapPersistent(VkDevice device);
    void Update(VkDevice device, const void *data, size_t size);
    void Destroy(VkDevice device);

    VkBuffer p_buffer;
    VkDeviceMemory p_memory;
    VkDeviceSize m_allocation_size;
};

} // namespace gouda::vk
