#pragma once

/**
 * @file gouda_vk_buffer_manager.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-15
 * @brief Engine module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <string>

#include <vulkan/vulkan.h>

#include "core/types.hpp"
#include "renderers/vulkan/vk_buffer.hpp"

namespace gouda::vk {

class Fence;
class Texture;
class Device;
class Queue;

class BufferManager {
public:
    BufferManager(Device *device, Queue *queue);
    ~BufferManager();

    // Create a generic buffer with specified usage and properties
    Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

    // Create a vertex buffer with staging
    Buffer CreateVertexBuffer(const void *data, VkDeviceSize size);

    Buffer CreateDynamicVertexBuffer(VkDeviceSize size);

    // Create a uniform buffer
    Buffer CreateUniformBuffer(size_t size);

    // Create a index buffer
    Buffer CreateIndexBuffer(const void *data, VkDeviceSize size);

    // Create a storage buffer
    Buffer CreateStorageBuffer(VkDeviceSize size);

    // Create and allocate memory for an image
    void CreateImage(Texture &texture, VkImageCreateInfo &image_info, VkMemoryPropertyFlags memory_properties);

    void CreateDepthImage(Texture &texture, ImageSize size, VkFormat format);

    // Utility to copy data between buffers
    void CopyBufferToBuffer(VkBuffer destination, VkBuffer source, VkDeviceSize size, VkCommandBuffer command_buffer);

    void CreateTextureImageFromData(Texture &texture, const void *pixels_ptr, ImageSize image_size,
                                    VkFormat texture_format, u32 layer_count, VkImageCreateFlags create_flags);

    // Texture-related methods
    std::unique_ptr<Texture> CreateTexture(std::string_view fileName); // TODO: add flags, mips and layers
    std::unique_ptr<Texture> CreateDefaultTexture();

    void CreateTextureImage(Texture &texture, ImageSize size, VkFormat format, u32 mipLevels, u32 layerCount,
                            VkImageCreateFlags flags);
    void UpdateTextureImage(Texture &texture, ImageSize size, VkFormat format, u32 layerCount, const void *data,
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

    Buffer CreateAndUploadStagingBuffer(const void *data, VkDeviceSize size);

private:
    Device *p_device;
    Queue *p_queue;

    VkCommandBuffer p_copy_command_buffer;
    VkCommandPool p_command_pool;
    std::unique_ptr<Fence> p_copy_fence;
};

} // namespace gouda::vk