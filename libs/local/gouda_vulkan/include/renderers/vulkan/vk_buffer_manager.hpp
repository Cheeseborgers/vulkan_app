#pragma once

/**
 * @file gouda_vk_buffer_manager.hpp
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
#include <optional>
#include <string>

#include <vulkan/vulkan.h>

#include "core/types.hpp"
#include "renderers/vulkan/vk_buffer.hpp"

namespace gouda {
namespace vk {

class VulkanTexture;
class VulkanDevice;
class VulkanQueue;

// TODO: Change to VulkanBufferManager
class BufferManager {
public:
    BufferManager(VulkanDevice *device, VulkanQueue *queue);
    ~BufferManager();

    // Create a generic buffer with specified usage and properties
    Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

    // Create a vertex buffer with staging
    Buffer CreateVertexBuffer(const void *vertices, size_t size);

    Buffer CreateDynamicVertexBuffer(VkDeviceSize size);

    // Create a uniform buffer
    Buffer CreateUniformBuffer(size_t size);

    // Create and allocate memory for an image
    void CreateImage(VulkanTexture &texture, VkImageCreateInfo &image_info, VkMemoryPropertyFlags memory_properties);

    void CreateDepthImage(VulkanTexture &texture, ImageSize size, VkFormat format);

    // Utility to copy data between buffers
    void CopyBufferToBuffer(VkBuffer destination, VkBuffer source, VkDeviceSize size, VkCommandBuffer command_buffer);

    void CreateTextureImageFromData(VulkanTexture &texture, const void *pixels_ptr, ImageSize image_size,
                                    VkFormat texture_format, u32 layer_count, VkImageCreateFlags create_flags);

    // Texture-related methods
    std::unique_ptr<VulkanTexture> CreateTexture(std::string_view fileName);
    void CreateTextureImage(VulkanTexture &texture, ImageSize size, VkFormat format, u32 mipLevels, u32 layerCount,
                            VkImageCreateFlags flags);
    void UpdateTextureImage(VulkanTexture &texture, ImageSize size, VkFormat format, u32 layerCount, const void *data,
                            VkImageLayout initialLayout);
    void CopyBufferToImage(VkBuffer source, VkImage destination, ImageSize imageSize, u32 layerCount);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                               u32 layerCount, u32 mipLevels);
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
                                VkImageViewType viewType, u32 layerCount, u32 mipLevels);
    VkSampler CreateTextureSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode);

private:
    // Helper to find suitable memory type
    Expect<u32, std::string> GetMemoryTypeIndex(u32 memory_type_bits, VkMemoryPropertyFlags required_properties);

    // Command buffer management for staging
    void BeginCommandBuffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage);
    void SubmitCopyCommand(VkCommandBuffer command_buffer);

private:
    VulkanDevice *p_device; // Reference to VulkanDevice for device and physical device properties
    VulkanQueue *p_queue;
    VkCommandBuffer p_copy_command_buffer; // For staging operations
    VkCommandPool p_command_pool;
    VkFence p_copy_fence;
};

} // namespace vk
} // namespace gouda