/**
 * @file gouda_vk_buffer_manager.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-15
 * @brief Engine vulkan buffer manager implementation
 */
#include "renderers/vulkan/vk_buffer_manager.hpp"

#include "debug/throw.hpp"
#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_queue.hpp"
#include "renderers/vulkan/vk_texture.hpp"
#include "utils/image.hpp"

#include "debug/logger.hpp"

namespace gouda::vk {

BufferManager::BufferManager(VulkanDevice *device, VulkanQueue *queue)
    : p_device(device),
      p_queue{queue},
      p_copy_command_buffer{VK_NULL_HANDLE},
      p_command_pool{VK_NULL_HANDLE},
      p_copy_fence{VK_NULL_HANDLE}
{
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = p_device->GetQueueFamily();

    VkResult result{vkCreateCommandPool(p_device->GetDevice(), &pool_info, nullptr, &p_command_pool)};
    CHECK_VK_RESULT(result, "vkCreateCommandPool in BufferManager");

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = p_command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(p_device->GetDevice(), &alloc_info, &p_copy_command_buffer);
    CHECK_VK_RESULT(result, "vkAllocateCommandBuffers in BufferManager");

    VkFenceCreateInfo fence_info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    result = vkCreateFence(p_device->GetDevice(), &fence_info, nullptr, &p_copy_fence);
    CHECK_VK_RESULT(result, "vkCreateFence in BufferManager");
}

BufferManager::~BufferManager()
{
    // Check we have a valid device
    if (p_device != VK_NULL_HANDLE) {

        // Destroy the fence if it exists
        if (p_copy_fence != VK_NULL_HANDLE) {
            vkDestroyFence(p_device->GetDevice(), p_copy_fence, nullptr);
            ENGINE_LOG_DEBUG("BufferManager copy fence destroyed.");
            p_copy_fence = VK_NULL_HANDLE; // Reset to avoid double deletion
        }

        // Free the command buffer if it exists
        if (p_copy_command_buffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(p_device->GetDevice(), p_command_pool, 1, &p_copy_command_buffer);
            ENGINE_LOG_DEBUG("BufferManager command buffer freed.");
        }

        // Destroy the command pool if it exists
        if (p_command_pool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(p_device->GetDevice(), p_command_pool, nullptr);
            ENGINE_LOG_DEBUG("BufferManager command pool destroyed.");
        }
    }
}

Buffer BufferManager::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo buffer_info{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                   .size = size,
                                   .usage = usage,
                                   .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

    Buffer buffer{};
    VkResult result{vkCreateBuffer(p_device->GetDevice(), &buffer_info, nullptr, &buffer.p_buffer)};
    CHECK_VK_RESULT(result, "vkCreateBuffer");

    VkMemoryRequirements mem_requirements{};
    vkGetBufferMemoryRequirements(p_device->GetDevice(), buffer.p_buffer, &mem_requirements);
    buffer.m_allocation_size = mem_requirements.size;

    auto memory_type_index = GetMemoryTypeIndex(mem_requirements.memoryTypeBits, properties);
    if (!memory_type_index) {
        ENGINE_THROW("Memory type selection failed: {}", memory_type_index.error());
    }

    VkMemoryAllocateInfo alloc_info{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                    .allocationSize = mem_requirements.size,
                                    .memoryTypeIndex = *memory_type_index};
    result = vkAllocateMemory(p_device->GetDevice(), &alloc_info, nullptr, &buffer.p_memory);
    CHECK_VK_RESULT(result, "vkAllocateMemory");

    vkBindBufferMemory(p_device->GetDevice(), buffer.p_buffer, buffer.p_memory, 0);
    return buffer;
}

Buffer BufferManager::CreateVertexBuffer(const void *vertices_ptr, VkDeviceSize size)
{
    Buffer staging_buffer{CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};
    staging_buffer.Update(p_device->GetDevice(), vertices_ptr, size);

    Buffer vertex_buffer{CreateBuffer(
        size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

    CopyBufferToBuffer(vertex_buffer.p_buffer, staging_buffer.p_buffer, size, p_copy_command_buffer);

    // Wait for the copy to complete
    vkWaitForFences(p_device->GetDevice(), 1, &p_copy_fence, VK_TRUE, UINT64_MAX);

    staging_buffer.Destroy(p_device->GetDevice());
    return vertex_buffer;
}

Buffer BufferManager::CreateDynamicVertexBuffer(VkDeviceSize size)
{
    return CreateBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

Buffer BufferManager::CreateUniformBuffer(size_t size)
{
    return CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

Buffer BufferManager::CreateIndexBuffer(const void *data, VkDeviceSize size)
{
    Buffer staging_buffer{CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};
    staging_buffer.Update(p_device->GetDevice(), data, size);

    Buffer index_buffer{CreateBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

    CopyBufferToBuffer(index_buffer.p_buffer, staging_buffer.p_buffer, size, p_copy_command_buffer);

    // Wait for the copy to complete
    vkWaitForFences(p_device->GetDevice(), 1, &p_copy_fence, VK_TRUE, UINT64_MAX);

    staging_buffer.Destroy(p_device->GetDevice());
    return index_buffer;
}

void BufferManager::CreateImage(VulkanTexture &texture, VkImageCreateInfo &image_info,
                                VkMemoryPropertyFlags memory_properties)
{
    VkResult result{vkCreateImage(p_device->GetDevice(), &image_info, nullptr, &texture.p_image)};
    CHECK_VK_RESULT(result, "vkCreateImage");

    VkMemoryRequirements memory_requirements{};
    vkGetImageMemoryRequirements(p_device->GetDevice(), texture.p_image, &memory_requirements);

    auto memory_type_index = GetMemoryTypeIndex(memory_requirements.memoryTypeBits, memory_properties);
    if (!memory_type_index) {
        ENGINE_THROW("Memory type selection failed: {}", memory_type_index.error());
    }

    VkMemoryAllocateInfo memory_allocate_info{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                              .allocationSize = memory_requirements.size,
                                              .memoryTypeIndex = *memory_type_index};

    result = vkAllocateMemory(p_device->GetDevice(), &memory_allocate_info, nullptr, &texture.p_memory);
    CHECK_VK_RESULT(result, "vkAllocateMemory");

    vkBindImageMemory(p_device->GetDevice(), texture.p_image, texture.p_memory, 0);
}

void BufferManager::CreateDepthImage(VulkanTexture &texture, ImageSize size, VkFormat format)
{
    VkImageCreateInfo image_info{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                 .imageType = VK_IMAGE_TYPE_2D,
                                 .format = format,
                                 .extent = {size.width, size.height, 1},
                                 .mipLevels = 1,
                                 .arrayLayers = 1,
                                 .samples = VK_SAMPLE_COUNT_1_BIT,
                                 .tiling = VK_IMAGE_TILING_OPTIMAL,
                                 .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                 .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                 .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

    CreateImage(texture, image_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void BufferManager::CopyBufferToBuffer(VkBuffer destination, VkBuffer source, VkDeviceSize size,
                                       VkCommandBuffer command_buffer)
{
    BeginCommandBuffer(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferCopy copy_region{.srcOffset = 0, .dstOffset = 0, .size = size};
    vkCmdCopyBuffer(command_buffer, source, destination, 1, &copy_region);

    SubmitCopyCommand(command_buffer);
}

void BufferManager::CreateTextureImageFromData(VulkanTexture &texture, const void *pixels_data_ptr,
                                               ImageSize image_size, VkFormat texture_format, u32 layer_count,
                                               VkImageCreateFlags create_flags)
{
    CreateTextureImage(texture, image_size, texture_format, 1, layer_count, create_flags);
    UpdateTextureImage(texture, image_size, texture_format, layer_count, pixels_data_ptr, VK_IMAGE_LAYOUT_UNDEFINED);
}

std::unique_ptr<VulkanTexture> BufferManager::CreateTexture(std::string_view fileName)
{
    auto imageResult = Image::Load(fileName, 4);
    if (!imageResult) {
        ENGINE_LOG_ERROR("Failed to load texture: {}", fileName);
        ENGINE_THROW("Failed to load texture: {}", fileName);
    }

    auto image = imageResult.value();
    auto texture = std::make_unique<VulkanTexture>();

    VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};
    CreateTextureImage(*texture, image.GetSize(), format, 1, 1, 0);
    UpdateTextureImage(*texture, image.GetSize(), format, 1, image.data().data(), VK_IMAGE_LAYOUT_UNDEFINED);
    texture->p_view = CreateImageView(texture->p_image, format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, 1);
    texture->p_sampler = CreateTextureSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);

    ENGINE_LOG_DEBUG("Created texture from: '{}'", fileName);
    return texture;
}

void BufferManager::CreateTextureImage(VulkanTexture &texture, ImageSize size, VkFormat format, u32 mip_levels,
                                       u32 layerCount, VkImageCreateFlags flags)
{
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = format;
    image_info.extent = VkExtent3D{static_cast<u32>(size.width), static_cast<u32>(size.height), 1};
    image_info.mipLevels = mip_levels;
    image_info.arrayLayers = layerCount;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.flags = flags;

    CreateImage(texture, image_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void BufferManager::UpdateTextureImage(VulkanTexture &texture, ImageSize size, VkFormat format, u32 layerCount,
                                       const void *data, VkImageLayout initialLayout)
{
    VkDeviceSize image_size{size.area() * 4}; // Assuming 4 bytes per pixel (RGBA)
    Buffer staging_buffer{CreateBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};

    staging_buffer.Update(p_device->GetDevice(), data, image_size);
    TransitionImageLayout(texture.p_image, format, initialLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount, 1);
    CopyBufferToImage(staging_buffer.p_buffer, texture.p_image, size, layerCount);
    TransitionImageLayout(texture.p_image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layerCount, 1);
    staging_buffer.Destroy(p_device->GetDevice());
}

void BufferManager::CopyBufferToImage(VkBuffer source, VkImage destination, ImageSize imageSize, u32 layerCount)
{
    BeginCommandBuffer(p_copy_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VkBufferImageCopy region{.bufferOffset = 0,
                             .bufferRowLength = 0,
                             .bufferImageHeight = 0,
                             .imageSubresource = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount},
                             .imageOffset = {0, 0, 0},
                             .imageExtent = {imageSize.width, imageSize.height, 1}};

    vkCmdCopyBufferToImage(p_copy_command_buffer, source, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &region);
    SubmitCopyCommand(p_copy_command_buffer);
}

void BufferManager::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout,
                                          VkImageLayout new_layout, u32 layerCount, u32 mip_levels)
{
    BeginCommandBuffer(p_copy_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkImageMemoryBarrier barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                 .oldLayout = old_layout,
                                 .newLayout = new_layout,
                                 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                 .image = image,
                                 .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_levels, 0, layerCount}};

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || (format == VK_FORMAT_D16_UNORM) ||
        (format == VK_FORMAT_X8_D24_UNORM_PACK32) || (format == VK_FORMAT_D32_SFLOAT) ||
        (format == VK_FORMAT_S8_UINT) || (format == VK_FORMAT_D16_UNORM_S8_UINT) ||
        (format == VK_FORMAT_D24_UNORM_S8_UINT)) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (HasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_GENERAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    // Convert back from read-only to updateable
    else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    // Convert from updateable texture to shader read-only
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    // Convert depth texture from undefined state to depth-stencil buffer
    else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    // Wait for render pass to complete
    else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0; // VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = 0;
        /*
                source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        ///		destination_stage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
                destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        */
        source_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    // Convert back from read-only to color attachment
    else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }

    // Convert from updateable texture to shader read-only
    else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    // Convert back from read-only to depth attachment
    else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destination_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }

    // Convert from updateable depth texture to shader read-only
    else if (old_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(p_copy_command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1,
                         &barrier);
    SubmitCopyCommand(p_copy_command_buffer);
}

VkImageView BufferManager::CreateImageView(VkImage image_ptr, VkFormat format, VkImageAspectFlags aspect_flags,
                                           VkImageViewType view_type, u32 layer_count, u32 mip_levels)
{
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.pNext = nullptr;
    view_info.flags = 0;
    view_info.image = image_ptr;
    view_info.viewType = view_type;
    view_info.format = format;
    view_info.components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                            .a = VK_COMPONENT_SWIZZLE_IDENTITY},
    view_info.subresourceRange = {.aspectMask = aspect_flags,
                                  .baseMipLevel = 0,
                                  .levelCount = mip_levels,
                                  .baseArrayLayer = 0,
                                  .layerCount = layer_count};

    VkImageView image_view{VK_NULL_HANDLE};
    VkResult result{vkCreateImageView(p_device->GetDevice(), &view_info, nullptr, &image_view)};
    if (result != VK_SUCCESS) {
        ENGINE_THROW("Failed to create image view in BufferManager");
    }
    return image_view;
}

VkSampler BufferManager::CreateTextureSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = magFilter;
    samplerInfo.minFilter = minFilter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = VK_FALSE; // Disable anisotropy for simplicity
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler sampler{VK_NULL_HANDLE};
    VkResult result{vkCreateSampler(p_device->GetDevice(), &samplerInfo, nullptr, &sampler)};
    if (result != VK_SUCCESS) {
        ENGINE_THROW("Failed to create texture sampler in BufferManager");
    }

    return sampler;
}

// Private functions -----------------------------------------------------------------------------------
Expect<u32, std::string> BufferManager::GetMemoryTypeIndex(u32 memory_type_bits,
                                                           VkMemoryPropertyFlags required_properties)
{
    const VkPhysicalDeviceMemoryProperties &mem_properties{p_device->GetSelectedPhysicalDevice().m_memory_properties};
    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((memory_type_bits & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & required_properties) == required_properties) {
            return i;
        }
    }
    return std::unexpected("Cannot find memory type for type: " + std::to_string(memory_type_bits));
}

void BufferManager::BeginCommandBuffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage)
{
    vkWaitForFences(p_device->GetDevice(), 1, &p_copy_fence, VK_TRUE, Constants::u64_max);
    VkCommandBufferBeginInfo begin_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = usage};
    vkBeginCommandBuffer(command_buffer, &begin_info);
}

void BufferManager::SubmitCopyCommand(VkCommandBuffer command_buffer)
{
    vkEndCommandBuffer(command_buffer);
    vkResetFences(p_device->GetDevice(), 1, &p_copy_fence);
    p_queue->Submit(command_buffer, p_copy_fence); // Wait for completion
}

} // namespace gouda::vk