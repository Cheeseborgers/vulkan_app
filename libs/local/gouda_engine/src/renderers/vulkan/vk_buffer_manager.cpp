/**
 * @file vk_buffer_manager.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-15
 * @brief Engine vulkan buffer manager implementation
 */
#include "renderers/vulkan/vk_buffer_manager.hpp"

#include "debug/logger.hpp"
#include "debug/throw.hpp"
#include "renderers/vulkan/vk_buffer.hpp"
#include "renderers/vulkan/vk_command_buffer_manager.hpp"
#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_fence.hpp"
#include "renderers/vulkan/vk_queue.hpp"
#include "renderers/vulkan/vk_texture.hpp"
#include "renderers/vulkan/vk_utils.hpp"
#include "utils/defer.hpp"
#include "utils/image.hpp"

namespace gouda::vk {

BufferManager::BufferManager(Device* device, Queue* queue, CommandBufferManager* command_buffer_manager)
    : p_device(device),
      p_queue{queue},
      p_command_buffer_manager{command_buffer_manager},
      p_copy_command_buffer{VK_NULL_HANDLE},
      p_command_pool{VK_NULL_HANDLE},
      p_copy_fence{nullptr}
{
    if (!p_device || !p_queue || !p_command_buffer_manager) {
        ENGINE_THROW("Invalid device, queue, or command buffer manager in BufferManager");
    }

    p_command_buffer_manager->AllocateBuffers(1, &p_copy_command_buffer);
    if (p_copy_command_buffer == VK_NULL_HANDLE) {
        ENGINE_THROW("Failed to allocate copy command buffer in BufferManager");
    }

    p_copy_fence = std::make_unique<Fence>(p_device);
    if (!p_copy_fence->Create(VK_FENCE_CREATE_SIGNALED_BIT)) {
        ENGINE_THROW("Failed to create copy fence.");
    }
}

BufferManager::~BufferManager()
{
    if (p_device != VK_NULL_HANDLE) {
        p_copy_fence.reset(); // Ensure fence is destroyed
    }
}

Buffer BufferManager::CreateAndUploadStagingBuffer(const void *data, const VkDeviceSize size) const
{
    const Buffer staging_buffer{
        CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};
    staging_buffer.Update(p_device->GetDevice(), data, size);
    return staging_buffer;
}

Buffer BufferManager::CreateBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage,
                                   const VkMemoryPropertyFlags properties) const
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Buffer buffer{};
    VkResult result{vkCreateBuffer(p_device->GetDevice(), &buffer_info, nullptr, &buffer.p_buffer)};
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateBuffer");
    }

    VkMemoryRequirements mem_requirements{};
    vkGetBufferMemoryRequirements(p_device->GetDevice(), buffer.p_buffer, &mem_requirements);
    buffer.m_allocation_size = mem_requirements.size;

    auto memory_type_index = GetMemoryTypeIndex(mem_requirements.memoryTypeBits, properties);
    if (!memory_type_index) {
        ENGINE_THROW("Memory type selection failed: {}", memory_type_index.error());
    }

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = *memory_type_index;

    result = vkAllocateMemory(p_device->GetDevice(), &alloc_info, nullptr, &buffer.p_memory);
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkAllocateMemory");
    }

    vkBindBufferMemory(p_device->GetDevice(), buffer.p_buffer, buffer.p_memory, 0);
    return buffer;
}

Buffer BufferManager::CreateVertexBuffer(const void *data, const VkDeviceSize size) const
{
    // Create a staging buffer and defer its destruction
    Buffer staging_buffer{CreateAndUploadStagingBuffer(data, size)};
    DEFER { staging_buffer.Destroy(p_device->GetDevice()); });

    const Buffer vertex_buffer{CreateBuffer(
        size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

    CopyBufferToBuffer(vertex_buffer.p_buffer, staging_buffer.p_buffer, size, p_copy_command_buffer);

    // Wait for the copy to complete
    p_copy_fence->WaitFor(constants::u64_max);

    return vertex_buffer;
}

Buffer BufferManager::CreateDynamicVertexBuffer(const VkDeviceSize size) const
{
    return CreateBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

Buffer BufferManager::CreateUniformBuffer(const size_t size) const
{
    return CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

Buffer BufferManager::CreateIndexBuffer(const void *data, const VkDeviceSize size) const
{
    // Create a staging buffer and defer its destruction
    Buffer staging_buffer{CreateAndUploadStagingBuffer(data, size)};
    DEFER { staging_buffer.Destroy(p_device->GetDevice()); });

    const Buffer index_buffer{CreateBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

    CopyBufferToBuffer(index_buffer.p_buffer, staging_buffer.p_buffer, size, p_copy_command_buffer);

    // Wait for the copy to complete
    p_copy_fence->WaitFor(constants::u64_max);

    return index_buffer;
}

Buffer BufferManager::CreateStorageBuffer(const VkDeviceSize size) const
{
    constexpr VkBufferUsageFlags usage{VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT};
    constexpr VkMemoryPropertyFlags properties{VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    return CreateBuffer(size, usage, properties);
}

void BufferManager::CreateImage(Texture &texture, const VkImageCreateInfo &image_info,
                                const VkMemoryPropertyFlags memory_properties) const
{
    VkResult result{vkCreateImage(p_device->GetDevice(), &image_info, nullptr, &texture.p_image)};
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateImage");
    }

    VkMemoryRequirements memory_requirements{};
    vkGetImageMemoryRequirements(p_device->GetDevice(), texture.p_image, &memory_requirements);

    auto memory_type_index = GetMemoryTypeIndex(memory_requirements.memoryTypeBits, memory_properties);
    if (!memory_type_index) {
        ENGINE_THROW("Memory type selection failed: {}", memory_type_index.error());
    }

    const VkMemoryAllocateInfo memory_allocate_info{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                                    .allocationSize = memory_requirements.size,
                                                    .memoryTypeIndex = *memory_type_index};

    result = vkAllocateMemory(p_device->GetDevice(), &memory_allocate_info, nullptr, &texture.p_memory);
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkAllocateMemory");
    }

    vkBindImageMemory(p_device->GetDevice(), texture.p_image, texture.p_memory, 0);
}

void BufferManager::CreateDepthImage(Texture &texture, const ImageSize size, const VkFormat format) const
{
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = format;
    image_info.extent = {static_cast<u32>(size.width), static_cast<u32>(size.height), 1};
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    CreateImage(texture, image_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void BufferManager::CopyBufferToBuffer(VkBuffer destination, VkBuffer source, const VkDeviceSize size,
                                       VkCommandBuffer command_buffer) const
{
    BeginCommandBuffer(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    const VkBufferCopy copy_region{.srcOffset = 0, .dstOffset = 0, .size = size};
    vkCmdCopyBuffer(command_buffer, source, destination, 1, &copy_region);

    SubmitCopyCommand(command_buffer);
}

void BufferManager::CreateTextureImageFromData(Texture &texture, const void *pixels_ptr, const ImageSize image_size,
                                               const VkFormat texture_format, const u32 layer_count,
                                               const VkImageCreateFlags create_flags) const
{
    CreateTextureImage(texture, image_size, texture_format, 1, layer_count, create_flags);
    UpdateTextureImage(texture, image_size, texture_format, layer_count, pixels_ptr, VK_IMAGE_LAYOUT_UNDEFINED);
}

std::unique_ptr<Texture> BufferManager::CreateTexture(StringView file_name, const u32 mips, const u32 layers) const
{
    const auto image_result = Image::Load(file_name, 4);
    if (!image_result) {
        ENGINE_LOG_ERROR("Failed to load texture: {}. Using default texture instead.", file_name);
        return CreateDefaultTexture(); // Fallback to default texture
    }

    const auto &image = image_result.value();
    auto texture = std::make_unique<Texture>();
    const VkFormat format{image_channels_to_vk_format(image.GetChannels())};

    CreateTextureImage(*texture, image.GetSize(), format, mips, layers, 0);
    UpdateTextureImage(*texture, image.GetSize(), format, layers, image.data().data(), VK_IMAGE_LAYOUT_UNDEFINED);
    texture->p_view =
        CreateImageView(texture->p_image, format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, layers, mips);
    texture->p_sampler =
        CreateTextureSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    return texture;
}

std::unique_ptr<Texture> BufferManager::CreateTexture(StringView file_name, const u32 mips, const u32 layers,
                                                      const VkImageCreateFlags create_flags, const VkFilter filter) const
{
    const auto image_result = Image::Load(file_name, 4);
    if (!image_result) {
        ENGINE_LOG_ERROR("Failed to load texture: {}. Using default texture instead.", file_name);
        return CreateDefaultTexture(); // Fallback to default texture
    }

    const auto &image = image_result.value();
    auto texture = std::make_unique<Texture>();
    const VkFormat format{image_channels_to_vk_format(image.GetChannels())};

    CreateTextureImage(*texture, image.GetSize(), format, mips, layers, create_flags);
    UpdateTextureImage(*texture, image.GetSize(), format, layers, image.data().data(), VK_IMAGE_LAYOUT_UNDEFINED);
    texture->p_view =
        CreateImageView(texture->p_image, format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, layers, mips);
    texture->p_sampler = CreateTextureSampler(filter, filter, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    return texture;
}

std::unique_ptr<Texture> BufferManager::CreateDefaultTexture() const
{
    // Create a 1x1 white texture
    constexpr u32 white_pixel{0xFFFFFFFF}; // RGBA white
    constexpr VkDeviceSize image_size{sizeof(u32)};

    // Create a staging buffer and defer its destruction
    Buffer staging_buffer{CreateAndUploadStagingBuffer(&white_pixel, image_size)};
    DEFER { staging_buffer.Destroy(p_device->GetDevice()); });

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.extent = {1, 1, 1};
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    auto texture = std::make_unique<Texture>();
    CreateImage(*texture, image_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Transition layout and copy data
    TransitionImageLayout(texture->p_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 1);
    CopyBufferToImage(staging_buffer.p_buffer, texture->p_image, ImageSize{1, 1}, 1);
    TransitionImageLayout(texture->p_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 1);

    // Create view and sampler
    texture->p_view = CreateImageView(texture->p_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT,
                                      VK_IMAGE_VIEW_TYPE_2D, 1, 1);
    texture->p_sampler = CreateTextureSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);

    staging_buffer.Destroy(p_device->GetDevice());

    return texture;
}

void BufferManager::CreateTextureImage(Texture &texture, const ImageSize size, const VkFormat format, const u32 mip_levels,
                                       const u32 layer_count, const VkImageCreateFlags flags) const
{
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = format;
    image_info.extent = VkExtent3D{static_cast<u32>(size.width), static_cast<u32>(size.height), 1};
    image_info.mipLevels = mip_levels;
    image_info.arrayLayers = layer_count;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.flags = flags;

    CreateImage(texture, image_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void BufferManager::UpdateTextureImage(const Texture &texture, const ImageSize size, const VkFormat format,
                                       const u32 layer_count, const void *data, const VkImageLayout initial_layout) const
{
    const u32 image_channel_count{vk_format_to_channel_count(format)};
    const VkDeviceSize image_size{size.area() * image_channel_count};

    // Create a staging buffer and defer its destruction
    Buffer staging_buffer{CreateAndUploadStagingBuffer(data, image_size)};
    DEFER { staging_buffer.Destroy(p_device->GetDevice()); });

    TransitionImageLayout(texture.p_image, format, initial_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layer_count,
                          1);
    CopyBufferToImage(staging_buffer.p_buffer, texture.p_image, size, layer_count);
    TransitionImageLayout(texture.p_image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layer_count, 1);
}

void BufferManager::CopyBufferToImage(VkBuffer source, VkImage destination, const ImageSize image_size,
                                      const u32 layer_count) const
{
    BeginCommandBuffer(p_copy_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    const VkBufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layer_count},
        .imageOffset = {0, 0, 0},
        .imageExtent = {static_cast<u32>(image_size.width), static_cast<u32>(image_size.height), 1}};

    vkCmdCopyBufferToImage(p_copy_command_buffer, source, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &region);
    SubmitCopyCommand(p_copy_command_buffer);
}

void BufferManager::TransitionImageLayout(VkImage image, const VkFormat format, const VkImageLayout old_layout,
                                          const VkImageLayout new_layout, const u32 layer_count,
                                          const u32 mip_levels) const
{
    BeginCommandBuffer(p_copy_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_levels, 0, layer_count};

    VkPipelineStageFlags source_stage{0};
    VkPipelineStageFlags destination_stage{0};

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || (format == VK_FORMAT_D16_UNORM) ||
        (format == VK_FORMAT_X8_D24_UNORM_PACK32) || (format == VK_FORMAT_D32_SFLOAT) ||
        (format == VK_FORMAT_S8_UINT) || (format == VK_FORMAT_D16_UNORM_S8_UINT) ||
        (format == VK_FORMAT_D24_UNORM_S8_UINT)) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (has_stencil_component(format)) {
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

    // Convert back from read-only to updatable
    else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    // Convert from updatable texture to shader read-only
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

    // Convert from updatable texture to shader read-only
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

    // Convert from updatable depth texture to shader read-only
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

VkImageView BufferManager::CreateImageView(VkImage image, const VkFormat format, const VkImageAspectFlags aspect_flags,
                                           const VkImageViewType view_type, const u32 layer_count,
                                           const u32 mip_levels) const
{
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.pNext = nullptr;
    view_info.flags = 0;
    view_info.image = image;
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
    if (const VkResult result{vkCreateImageView(p_device->GetDevice(), &view_info, nullptr, &image_view)};
        result != VK_SUCCESS) {
        ENGINE_THROW("Failed to create image view in BufferManager");
    }
    return image_view;
}

VkSampler BufferManager::CreateTextureSampler(const VkFilter minFilter, const VkFilter magFilter,
                                              const VkSamplerAddressMode addressMode) const
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
    if (const VkResult result{vkCreateSampler(p_device->GetDevice(), &samplerInfo, nullptr, &sampler)};
        result != VK_SUCCESS) {
        ENGINE_THROW("Failed to create texture sampler in BufferManager");
    }

    return sampler;
}

// Private functions -----------------------------------------------------------------------------------
Expect<u32, String> BufferManager::GetMemoryTypeIndex(const u32 memory_type_bits,
                                                           const VkMemoryPropertyFlags required_properties) const
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

void BufferManager::BeginCommandBuffer(VkCommandBuffer command_buffer, const VkCommandBufferUsageFlags usage) const
{
    p_copy_fence->WaitFor(constants::u64_max);
    const VkCommandBufferBeginInfo begin_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = usage};
    vkBeginCommandBuffer(command_buffer, &begin_info);
}

void BufferManager::SubmitCopyCommand(VkCommandBuffer command_buffer) const
{
    vkEndCommandBuffer(command_buffer);
    p_copy_fence->Reset();

    if (p_queue) {
        p_queue->Submit(command_buffer, p_copy_fence.get()); // Wait for completion
    }
}

} // namespace gouda::vk