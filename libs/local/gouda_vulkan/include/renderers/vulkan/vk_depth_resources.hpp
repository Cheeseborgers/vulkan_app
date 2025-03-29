#pragma once

/**
 * @file vk_depth_resources.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-19
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
#include <vector>

#include "renderers/vulkan/vk_texture.hpp"

namespace gouda::vk {

class VulkanSwapchain;
class VulkanDevice;
class VulkanInstance;
class BufferManager;

class VulkanDepthResources {
public:
    VulkanDepthResources(VulkanDevice *device, VulkanInstance *instance, BufferManager *buffer_manager,
                         VulkanSwapchain *swapchain);
    ~VulkanDepthResources();

    void Create();
    void Recreate();
    void Destroy();

    const std::vector<VulkanTexture> &GetDepthImages() const { return m_depth_images; }

private:
    VulkanSwapchain *p_swapchain;
    VulkanDevice *p_device;
    VulkanInstance *p_instance;
    BufferManager *p_buffer_manager;

    std::vector<VulkanTexture> m_depth_images;
};

} // namespace gouda