#pragma once
/**
 * @file vk_buffer_manager.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-15
 * @brief Engine Vulkan buffer manager module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <memory>

#include <vulkan/vulkan.h>

#include "core/types.hpp"

namespace gouda::vk {

class Buffer;
class CommandBufferManager;
class Fence;
class Texture;
class Device;
class Queue;

class BufferManager {
public:
    BufferManager(Device* device, Queue* queue, CommandBufferManager* command_buffer_manager);
    ~BufferManager();

    // Create a generic buffer with specified usage and properties
    [[nodiscard]] Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const;

    // Create a vertex buffer with staging
    [[nodiscard]] Buffer CreateVertexBuffer(const void *data, VkDeviceSize size) const;

    [[nodiscard]] Buffer CreateDynamicVertexBuffer(VkDeviceSize size) const;

    // Create a uniform buffer
    [[nodiscard]] Buffer CreateUniformBuffer(size_t size) const;

    // Create an index buffer
    [[nodiscard]] Buffer CreateIndexBuffer(const void *data, VkDeviceSize size) const;

    // Create a storage buffer
    [[nodiscard]] Buffer CreateStorageBuffer(VkDeviceSize size) const;

    // Create and allocate memory for an image
    void CreateImage(Texture &texture, const VkImageCreateInfo &image_info, VkMemoryPropertyFlags memory_properties) const;

    void CreateDepthImage(Texture &texture, ImageSize size, VkFormat format) const;

    // Utility to copy data between buffers
    void CopyBufferToBuffer(VkBuffer destination, VkBuffer source, VkDeviceSize size, VkCommandBuffer command_buffer) const;

    void CreateTextureImageFromData(Texture &texture, const void *pixels_ptr, ImageSize image_size,
                                    VkFormat texture_format, u32 layer_count, VkImageCreateFlags create_flags) const;

    // Texture-related methods
    [[nodiscard]] std::unique_ptr<Texture> CreateTexture(StringView file_name, u32 mips = 1, u32 layers = 1) const;
    [[nodiscard]] std::unique_ptr<Texture> CreateTexture(StringView file_name, u32 mips, u32 layers, VkImageCreateFlags create_flags,
                                           VkFilter filter) const;
    [[nodiscard]] std::unique_ptr<Texture> CreateDefaultTexture() const;

    void CreateTextureImage(Texture &texture, ImageSize size, VkFormat format, u32 mipLevels, u32 layerCount,
                            VkImageCreateFlags flags) const;
    void UpdateTextureImage(const Texture &texture, ImageSize size, VkFormat format, u32 layerCount, const void *data,
                            VkImageLayout initialLayout) const;
    void CopyBufferToImage(VkBuffer source, VkImage destination, ImageSize imageSize, u32 layerCount) const;
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                               u32 layerCount, u32 mipLevels) const;
    [[nodiscard]] VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
                                VkImageViewType viewType, u32 layerCount, u32 mipLevels) const;
    [[nodiscard]] VkSampler CreateTextureSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode) const;

private:
    // Helper to find suitable memory type
    [[nodiscard]] Expect<u32, String> GetMemoryTypeIndex(u32 memory_type_bits, VkMemoryPropertyFlags required_properties) const;

    // Command buffer management for staging
    void BeginCommandBuffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage) const;
    void SubmitCopyCommand(VkCommandBuffer command_buffer) const;

    [[nodiscard]] Buffer CreateAndUploadStagingBuffer(const void *data, VkDeviceSize size) const;

private:
    Device *p_device;
    Queue *p_queue;
    CommandBufferManager *p_command_buffer_manager;

    VkCommandBuffer p_copy_command_buffer;
    VkCommandPool p_command_pool;
    std::unique_ptr<Fence> p_copy_fence;
};

} // namespace gouda::vk