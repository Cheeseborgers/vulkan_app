#pragma once

/**
 * @file gouda_vk_buffer.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-15
 * @brief Engine module
 *
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
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
