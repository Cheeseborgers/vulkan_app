#include "gouda_vk_core.hpp"

#include <print>
#include <sstream>
#include <vector>

#include "gouda_throw.hpp"
#include "logger.hpp"

#include "gouda_assert.hpp"

#include "utility/image.hpp"

#include "gouda_vk_utils.hpp"
#include "gouda_vk_wrapper.hpp"

namespace GoudaVK {

VulkanCore::VulkanCore()
    : m_is_initialized{false},
      m_vsync_mode{VSyncMode::DISABLED},
      p_instance{nullptr},
      p_device{nullptr},
      p_window{nullptr},
      m_swap_chain_surface_format{},
      p_swap_chain{VK_NULL_HANDLE},
      p_command_buffer_pool{VK_NULL_HANDLE},
      m_queue{},
      p_copy_command_buffer{VK_NULL_HANDLE}
{
}

VulkanCore::~VulkanCore()
{
    if (m_is_initialized) {

        ENGINE_LOG_INFO("Cleaning up Vulkan");

        // Destroy in this order!!!
        vkFreeCommandBuffers(p_device->GetDevice(), p_command_buffer_pool, 1, &p_copy_command_buffer);
        ENGINE_LOG_INFO("Command buffers destroyed");

        vkDestroyCommandPool(p_device->GetDevice(), p_command_buffer_pool, nullptr);
        ENGINE_LOG_INFO("Command pool destroyed");

        m_queue.Destroy();
        ENGINE_LOG_INFO("Queue buffer destroyed");

        for (auto image_view : m_image_views) {
            vkDestroyImageView(p_device->GetDevice(), image_view, nullptr);
        }

        m_image_views.clear();

        vkDestroySwapchainKHR(p_device->GetDevice(), p_swap_chain, nullptr);

        ENGINE_LOG_INFO("Swapchain destroyed");
    }
}

void VulkanCore::Init(GLFWwindow *window_ptr, std::string_view app_name, SemVer vulkan_api_version,
                      VSyncMode vsync_mode)
{
    ASSERT(window_ptr, "Window pointer cannot be null");

    p_window = window_ptr;
    m_vsync_mode = vsync_mode;

    p_instance = std::make_unique<VulkanInstance>(app_name, vulkan_api_version, p_window);
    ENGINE_LOG_INFO("VulkanInstance initialized");

    p_device = std::make_unique<VulkanDevice>(*p_instance, VK_QUEUE_GRAPHICS_BIT);
    ENGINE_LOG_INFO("VulkanDevice initialized");

    CreateSwapchain();
    CreateSwapchainImageViews();

    CreateCommandBufferPool();

    m_queue.Init(p_device->GetDevice(), &p_swap_chain, p_device->GetQueueFamily(), 0);

    CreateCommandBuffers(1, &p_copy_command_buffer);

    m_is_initialized = true;
}

VkRenderPass VulkanCore::CreateSimpleRenderPass()
{
    VkAttachmentDescription colour_attachment_description{};
    colour_attachment_description.flags = 0;
    colour_attachment_description.format = m_swap_chain_surface_format.format;
    colour_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    colour_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colour_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colour_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colour_attachment_reference = {.attachment = 0,
                                                         .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass_description{};
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &colour_attachment_reference;
    subpass_description.pResolveAttachments = nullptr;
    subpass_description.pDepthStencilAttachment = nullptr;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = nullptr;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &colour_attachment_description;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 0;
    render_pass_create_info.pDependencies = nullptr;

    VkRenderPass render_pass;

    VkResult result{vkCreateRenderPass(p_device->GetDevice(), &render_pass_create_info, nullptr, &render_pass)};
    CHECK_VK_RESULT(result, "vkCreateRenderPass\n");

    ENGINE_LOG_INFO("Simple render pass created");

    return render_pass;
}

std::vector<VkFramebuffer> VulkanCore::CreateFramebuffers(VkRenderPass render_pass_ptr) const
{
    // Vector to hold the created framebuffers, resized to match the number of images
    std::vector<VkFramebuffer> frame_buffers;
    frame_buffers.resize(m_images.size());

    // Retrieve the dimensions of the framebuffer
    FrameBufferSize frame_buffer_size;
    GetFramebufferSize(frame_buffer_size.width, frame_buffer_size.height);

    ENGINE_LOG_INFO("Framebuffer size retrieved: {}x{}", frame_buffer_size.width, frame_buffer_size.height);

    // Ensure the framebuffer size is valid (non-zero area)
    ASSERT(frame_buffer_size.area() > 0, "Framebuffer dimensions cannot be zero");

    // Loop through each image in the swapchain to create a framebuffer
    for (uint i = 0; i < m_images.size(); i++) {

        // Define framebuffer creation settings
        VkFramebufferCreateInfo frame_buffer_create_info{};
        frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buffer_create_info.renderPass = render_pass_ptr;     // Attach to the given render pass
        frame_buffer_create_info.attachmentCount = 1;              // Single image attachment
        frame_buffer_create_info.pAttachments = &m_image_views[i]; // Attach the corresponding image view
        frame_buffer_create_info.width = static_cast<u32>(frame_buffer_size.width);   // Set framebuffer width
        frame_buffer_create_info.height = static_cast<u32>(frame_buffer_size.height); // Set framebuffer height
        frame_buffer_create_info.layers = 1;

        // Create the framebuffer and check for errors
        VkResult result{
            vkCreateFramebuffer(p_device->GetDevice(), &frame_buffer_create_info, nullptr, &frame_buffers[i])};
        CHECK_VK_RESULT(result, "vkCreateFramebuffer\n");
    }

    ENGINE_LOG_INFO("Framebuffers created");

    return frame_buffers;
}

void VulkanCore::DestroyFramebuffers(std::vector<VkFramebuffer> &frame_buffers)
{
    for (auto buffer : frame_buffers) {
        vkDestroyFramebuffer(p_device->GetDevice(), buffer, nullptr);
    }

    frame_buffers.clear();

    ENGINE_LOG_INFO("Framebuffers destroyed");
}

void VulkanCore::CreateCommandBuffers(u32 count, VkCommandBuffer *command_buffers_ptr)
{
    VkCommandBufferAllocateInfo command_buffer_allocation_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                               .pNext = nullptr,
                                                               .commandPool = p_command_buffer_pool,
                                                               .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                               .commandBufferCount = count};

    VkResult result{
        vkAllocateCommandBuffers(p_device->GetDevice(), &command_buffer_allocation_info, command_buffers_ptr)};
    CHECK_VK_RESULT(result, "vkAllocateCommandBuffers\n");

    ENGINE_LOG_INFO("Command buffer(s) created: {}", count);
}

void VulkanCore::FreeCommandBuffers(u32 count, const VkCommandBuffer *command_buffers_ptr)
{
    m_queue.WaitIdle();
    vkFreeCommandBuffers(p_device->GetDevice(), p_command_buffer_pool, count, command_buffers_ptr);

    ENGINE_LOG_INFO("Command buffer(s) freed: {}", count);
}

AllocatedBuffer VulkanCore::CreateVertexBuffer(const void *vertices_ptr, size_t size)
{
    // Create a staging buffer with host-visible memory
    AllocatedBuffer staging_vertex_buffer =
        CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Map memory and copy data
    void *memory_ptr{nullptr};
    VkDeviceSize offset{0};
    VkMemoryMapFlags flags{0};
    VkResult result{vkMapMemory(p_device->GetDevice(), staging_vertex_buffer.p_memory, offset,
                                staging_vertex_buffer.m_allocation_size, flags, &memory_ptr)};
    CHECK_VK_RESULT(result, "vkMapMemory\n");

    memcpy(memory_ptr, vertices_ptr, size);
    vkUnmapMemory(p_device->GetDevice(), staging_vertex_buffer.p_memory);

    // Create the actual GPU vertex buffer (device local)
    AllocatedBuffer vertex_buffer{CreateBuffer(size,
                                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

    // Copy to buffer (this is asynchronous)
    CopyBufferToBuffer(vertex_buffer.p_buffer, staging_vertex_buffer.p_buffer, size);

    // Ensure the copy operation completes before destroying the staging buffer
    m_queue.WaitIdle();

    // Now it is safe to destroy the staging buffer
    vkDestroyBuffer(p_device->GetDevice(), staging_vertex_buffer.p_buffer, nullptr);
    vkFreeMemory(p_device->GetDevice(), staging_vertex_buffer.p_memory, nullptr);

    return vertex_buffer;
}

void VulkanCore::CreateUniformBuffers(std::vector<AllocatedBuffer> &uniform_buffers, size_t data_size)
{
    uniform_buffers.clear();                  // Clear the existing vector before filling it
    uniform_buffers.reserve(m_images.size()); // Reserve space to avoid reallocations

    for (const auto &image : m_images) {
        uniform_buffers.emplace_back(CreateUniformBuffer(data_size));
    }
}

std::vector<AllocatedBuffer> VulkanCore::CreateUniformBuffers(size_t data_size)
{
    std::vector<AllocatedBuffer> uniform_buffers;

    CreateUniformBuffers(uniform_buffers, data_size);

    return uniform_buffers;
}

VkImageView CreateImageView(VkDevice device_ptr, VkImage image_ptr, VkFormat format, VkImageAspectFlags aspect_flags,
                            VkImageViewType view_type, u32 layer_count, u32 mip_levels)
{
    VkImageViewCreateInfo view_info{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                    .pNext = nullptr,
                                    .flags = 0,
                                    .image = image_ptr,
                                    .viewType = view_type,
                                    .format = format,
                                    .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                                   .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                                   .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                                   .a = VK_COMPONENT_SWIZZLE_IDENTITY},
                                    .subresourceRange = {.aspectMask = aspect_flags,
                                                         .baseMipLevel = 0,
                                                         .levelCount = mip_levels,
                                                         .baseArrayLayer = 0,
                                                         .layerCount = layer_count}};

    VkImageView image_view;
    VkResult result{vkCreateImageView(device_ptr, &view_info, nullptr, &image_view)};
    CHECK_VK_RESULT(result, "vkCreateImageView");

    return image_view;
}

void VulkanCore::CreateTexture(std::string_view file_name, VulkanTexture &texture)
{
    StbiImage image(file_name);

    if (!image.GetData()) {
        ENGINE_LOG_ERROR("Error loading texture from: {}", file_name);
        ENGINE_THROW("Could not load texture image");
    }

    u32 layer_count{1};
    VkImageCreateFlags flags{0};
    VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};
    CreateTextureImageFromData(texture, image.GetData(), image.GetSize(), format, layer_count, flags);

    image.free();

    VkImageViewType image_view_type = VK_IMAGE_VIEW_TYPE_2D;
    VkImageAspectFlags image_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
    u32 mip_levels{1};

    texture.p_view = CreateImageView(p_device->GetDevice(), texture.p_image, format, image_aspect_flags,
                                     image_view_type, layer_count, mip_levels);

    VkFilter min_filter{VK_FILTER_LINEAR};
    VkFilter max_filter{VK_FILTER_LINEAR};
    VkSamplerAddressMode address_mode{VK_SAMPLER_ADDRESS_MODE_REPEAT};

    texture.p_sampler = CreateTextureSampler(p_device->GetDevice(), min_filter, max_filter, address_mode);

    ENGINE_LOG_INFO("Created texture from: {}", file_name);
}

const VkImage &VulkanCore::GetImage(int index)
{
    if (static_cast<size_t>(index) >= m_images.size()) {
        ENGINE_LOG_FATAL("Invalid image index: {}", index);
        ENGINE_THROW("Invalid image index!");
    }

    return m_images[static_cast<size_t>(index)];
}

void VulkanCore::GetFramebufferSize(int &width, int &height) const { glfwGetWindowSize(p_window, &width, &height); }

// Private -----------------------------------------------------------------------------------------------------------

static VkPresentModeKHR GetVulkanPresentMode(VSyncMode mode)
{
    switch (mode) {
        case VSyncMode::DISABLED:
            return VK_PRESENT_MODE_IMMEDIATE_KHR; // No sync, high FPS
        case VSyncMode::ENABLED:
            return VK_PRESENT_MODE_FIFO_KHR; // Standard V-Sync
        case VSyncMode::MAILBOX:
            return VK_PRESENT_MODE_MAILBOX_KHR; // Adaptive V-Sync
        default:
            return VK_PRESENT_MODE_FIFO_KHR;
    }
}

static VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> &present_modes, VSyncMode vsync_mode)
{
    if (vsync_mode == VSyncMode::ENABLED) {
        return VK_PRESENT_MODE_FIFO_KHR; // FIFO is always supported and enables V-Sync
    }

    for (auto mode : present_modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode; // Use Mailbox if available (low-latency V-Sync)
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR; // Fallback to FIFO which is always supported
}

static u32 ChooseNumImages(const VkSurfaceCapabilitiesKHR &capabilities)
{
    u32 requested_number_of_images{capabilities.minImageCount + 1};
    u32 final_number_of_images{0};

    if ((capabilities.maxImageCount > 0) && (requested_number_of_images > capabilities.maxImageCount)) {
        final_number_of_images = capabilities.maxImageCount;
    }
    else {
        final_number_of_images = requested_number_of_images;
    }

    return final_number_of_images;
}

static VkSurfaceFormatKHR ChooseSurfaceFormatAndColorSpace(const std::vector<VkSurfaceFormatKHR> &surface_formats)
{
    for (auto surface_format : surface_formats) {
        if ((surface_format.format == VK_FORMAT_B8G8R8A8_SRGB) &&
            (surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
            return surface_format;
        }
    }

    return surface_formats[0];
}

void VulkanCore::CreateSwapchain()
{
    const VkSurfaceCapabilitiesKHR &surface_capabiltities{p_device->GetSelectedPhysicalDevice().m_surface_capabilties};

    const u32 number_of_images{ChooseNumImages(surface_capabiltities)};

    const std::vector<VkPresentModeKHR> &present_modes{p_device->GetSelectedPhysicalDevice().m_present_modes};
    VkPresentModeKHR present_mode{ChoosePresentMode(present_modes, m_vsync_mode)};

    m_swap_chain_surface_format =
        ChooseSurfaceFormatAndColorSpace(p_device->GetSelectedPhysicalDevice().m_surface_formats);

    FrameBufferSize framebuffer_size;
    GetFramebufferSize(framebuffer_size.width, framebuffer_size.height);

    // Log the capabilities for debugging
    ENGINE_LOG_INFO("Surface capabilities: currentExtent = {}x{}, minImageExtent = {}x{}, maxImageExtent = {}x{}",
                    surface_capabiltities.currentExtent.width, surface_capabiltities.currentExtent.height,
                    surface_capabiltities.minImageExtent.width, surface_capabiltities.minImageExtent.height,
                    surface_capabiltities.maxImageExtent.width, surface_capabiltities.maxImageExtent.height);

    // Choose an appropriate extent
    VkExtent2D extent;
    if (surface_capabiltities.currentExtent.width != UINT32_MAX) {
        // If currentExtent is valid, use it (some platforms mandate this)
        extent = surface_capabiltities.currentExtent;
    }

    ENGINE_LOG_INFO("Selected swapchain extent: {}x{}", extent.width, extent.height);

    // FIXME: This is unsafe. store in swapchain ish when we refactor it
    u32 queue_family_index{p_device->GetQueueFamily()};

    VkSwapchainCreateInfoKHR swap_chain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = p_instance->GetSurface(),
        .minImageCount = number_of_images,
        .imageFormat = m_swap_chain_surface_format.format,
        .imageColorSpace = m_swap_chain_surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT),
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queue_family_index, // FIXME: This is unsafe. store in swapchain ish when we refactor it
        .preTransform = surface_capabiltities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = nullptr};

    VkResult result{vkCreateSwapchainKHR(p_device->GetDevice(), &swap_chain_create_info, nullptr, &p_swap_chain)};
    CHECK_VK_RESULT(result, "vkCreateSwapchainKHR\n");

    ENGINE_LOG_INFO("Swap chain created with V-Sync mode: {}", (m_vsync_mode == VSyncMode::ENABLED) ? "ENABLED (FIFO)"
                                                               : (m_vsync_mode == VSyncMode::MAILBOX)
                                                                   ? "DISABLED (MAILBOX)"
                                                                   : "DISABLED (IMMEDIATE)");
}

void VulkanCore::CreateSwapchainImageViews()
{
    uint number_of_swap_chain_images{0};
    VkResult result{
        vkGetSwapchainImagesKHR(p_device->GetDevice(), p_swap_chain, &number_of_swap_chain_images, nullptr)};
    CHECK_VK_RESULT(result, "vkGetSwapchainImagesKHR\n");

    const VkSurfaceCapabilitiesKHR &surface_capabiltities{p_device->GetSelectedPhysicalDevice().m_surface_capabilties};

    const u32 number_of_images{ChooseNumImages(surface_capabiltities)};

    ASSERT(number_of_images == number_of_swap_chain_images,
           "Number of images is not equal to the number of swap chain images");

    ENGINE_LOG_INFO("Created {} swap chain images", number_of_swap_chain_images);

    m_images.resize(number_of_swap_chain_images);
    m_image_views.resize(number_of_swap_chain_images);

    result =
        vkGetSwapchainImagesKHR(p_device->GetDevice(), p_swap_chain, &number_of_swap_chain_images, m_images.data());
    CHECK_VK_RESULT(result, "vkGetSwapchainImagesKHR\n");

    u32 layer_count{1};
    u32 mip_levels{1};
    for (u32 i = 0; i < number_of_swap_chain_images; i++) {
        m_image_views[i] = CreateImageView(p_device->GetDevice(), m_images[i], m_swap_chain_surface_format.format,
                                           VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, layer_count, mip_levels);
    }
}

void VulkanCore::CreateCommandBufferPool()
{
    VkCommandPoolCreateInfo command_pool_create_info{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                                     .pNext = nullptr,
                                                     .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                                     .queueFamilyIndex = p_device->GetQueueFamily()};

    VkResult result{
        vkCreateCommandPool(p_device->GetDevice(), &command_pool_create_info, nullptr, &p_command_buffer_pool)};
    CHECK_VK_RESULT(result, "vkCreateCommandPool\n");

    ENGINE_LOG_INFO("Command buffer pool created");
}

u32 VulkanCore::GetMemoryTypeIndex(u32 memory_type_bits, VkMemoryPropertyFlags required_memory_property_flags)
{
    // FIXME: Sort logging and program exit here
    const VkPhysicalDeviceMemoryProperties &memory_properties{
        p_device->GetSelectedPhysicalDevice().m_memory_properties};

    for (uint i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((memory_type_bits & (1 << i)) && ((memory_properties.memoryTypes[i].propertyFlags &
                                               required_memory_property_flags) == required_memory_property_flags)) {
            return i;
        }
    }

    ENGINE_LOG_FATAL("Cannot find memory type for type: {}, requested memory properties: {}", memory_type_bits,
                     required_memory_property_flags);
    exit(1);
    return 0; // TODO: handle this better than returing 0 as it exits the application before. maybe return
              // std::optional
}

void VulkanCore::CopyBufferToBuffer(VkBuffer destination, VkBuffer source, VkDeviceSize size)
{
    BeginCommandBuffer(p_copy_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferCopy buffer_copy{};
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = size;

    vkCmdCopyBuffer(p_copy_command_buffer, source, destination, 1, &buffer_copy);

    SubmitCopyCommand();
}

AllocatedBuffer GoudaVK::VulkanCore::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                                  VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo vertex_buffer_create_info{};
    vertex_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_create_info.size = size;
    vertex_buffer_create_info.usage = usage;
    vertex_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    AllocatedBuffer buffer;

    // create a buffer
    VkResult result{vkCreateBuffer(p_device->GetDevice(), &vertex_buffer_create_info, nullptr, &buffer.p_buffer)};
    CHECK_VK_RESULT(result, "vkCreateBuffer\n");

    // Get the buffer memory requirements
    VkMemoryRequirements memory_requirements{};
    vkGetBufferMemoryRequirements(p_device->GetDevice(), buffer.p_buffer, &memory_requirements);

    buffer.m_allocation_size = memory_requirements.size;

    // Get the memory type index
    u32 memory_type_index{GetMemoryTypeIndex(memory_requirements.memoryTypeBits, properties)};

    // Allocate memory
    VkMemoryAllocateInfo memory_allocate_info{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                              .pNext = nullptr,
                                              .allocationSize = memory_requirements.size,
                                              .memoryTypeIndex = memory_type_index};

    result = vkAllocateMemory(p_device->GetDevice(), &memory_allocate_info, nullptr, &buffer.p_memory);
    CHECK_VK_RESULT(result, "vkAllocateMemory error %d\n");

    // Bind memory
    result = vkBindBufferMemory(p_device->GetDevice(), buffer.p_buffer, buffer.p_memory, 0);
    CHECK_VK_RESULT(result, "vkBindBufferMemory error %d\n");

    return buffer;
}

AllocatedBuffer VulkanCore::CreateUniformBuffer(size_t size)
{
    VkBufferUsageFlags usage{VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT};
    VkMemoryPropertyFlags memory_properties{VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    AllocatedBuffer buffer{CreateBuffer(size, usage, memory_properties)};

    if (buffer.p_buffer == VK_NULL_HANDLE) {
        ENGINE_LOG_ERROR("Failed to create uniform buffer");
        std::terminate();
    }

    return buffer;
}

void VulkanCore::CreateTextureImageFromData(VulkanTexture &texture, const void *pixels_data_ptr, ImageSize image_size,
                                            VkFormat texture_format, u32 layer_count, VkImageCreateFlags create_flags)
{
    CreateImage(texture, image_size, texture_format, VK_IMAGE_TILING_OPTIMAL,
                (VkImageUsageFlagBits)(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, create_flags, 1);

    UpdateTextureImage(texture, image_size, texture_format, layer_count, pixels_data_ptr, VK_IMAGE_LAYOUT_UNDEFINED);
}

void VulkanCore::CreateImage(VulkanTexture &texture, ImageSize image_size, VkFormat texture_format,
                             VkImageTiling image_tiling, VkImageUsageFlags usage_flags,
                             VkMemoryPropertyFlagBits memory_property_flags, VkImageCreateFlags create_flags,
                             u32 mip_levels)
{
    VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = create_flags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = texture_format,
        .extent = VkExtent3D{.width = image_size.width, .height = image_size.height, .depth = 1},
        .mipLevels = mip_levels,
        .arrayLayers = static_cast<u32>((create_flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? 6 : 1),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = image_tiling,
        .usage = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

    ENGINE_LOG_DEBUG("Created Image {} width: {}", image_size.width, image_size.width);

    VkResult result{vkCreateImage(p_device->GetDevice(), &image_create_info, nullptr, &texture.p_image)};
    CHECK_VK_RESULT(result, "vkCreateImage error");

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(p_device->GetDevice(), texture.p_image, &memory_requirements);

    u32 memory_type_index{GetMemoryTypeIndex(memory_requirements.memoryTypeBits, memory_property_flags)};

    VkMemoryAllocateInfo memory_allocate_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                                 .pNext = nullptr,
                                                 .allocationSize = memory_requirements.size,
                                                 .memoryTypeIndex = memory_type_index};

    result = vkAllocateMemory(p_device->GetDevice(), &memory_allocate_info, nullptr, &texture.p_memory);
    CHECK_VK_RESULT(result, "vkAllocateMemory error");

    vkBindImageMemory(p_device->GetDevice(), texture.p_image, texture.p_memory, 0);
}

void VulkanCore::UpdateTextureImage(VulkanTexture &texture, ImageSize image_size, VkFormat texture_format,
                                    u32 layer_count, const void *pixels_data_ptr, VkImageLayout source_image_layout)
{
    int bytes_per_pixel{GetBytesPerTextureFormat(texture_format)};
    ASSERT(bytes_per_pixel > 0, "Texture bytes per pixel is zero");

    if (bytes_per_pixel == 0) {
        ENGINE_LOG_ERROR("Texture bytes per pixel is zero");
        throw std::runtime_error("Texture bytes per pixel is zero");
    }

    VkDeviceSize layer_size{image_size.area() * bytes_per_pixel};
    VkDeviceSize final_memory_size{layer_size * layer_count};

    VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags memory_property_flags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    AllocatedBuffer staging_texture = CreateBuffer(final_memory_size, buffer_usage_flags, memory_property_flags);

    staging_texture.Update(p_device->GetDevice(), pixels_data_ptr, final_memory_size);

    TransitionImageLayout(texture.p_image, texture_format, source_image_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          layer_count, 1);

    CopyBufferToImage(texture.p_image, staging_texture.p_buffer, image_size, layer_count);
    TransitionImageLayout(texture.p_image, texture_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layer_count, 1);

    staging_texture.Destroy(p_device->GetDevice());
}

void VulkanCore::CopyBufferToImage(VkImage destination, VkBuffer source, ImageSize image_size, u32 layer_count)
{
    BeginCommandBuffer(p_copy_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource =
            VkImageSubresourceLayers{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = layer_count},
        .imageOffset = VkOffset3D{.x = 0, .y = 0, .z = 0},
        .imageExtent = VkExtent3D{.width = image_size.width, .height = image_size.height, .depth = 1}};

    vkCmdCopyBufferToImage(p_copy_command_buffer, source, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &region);

    SubmitCopyCommand();
}

void VulkanCore::SubmitCopyCommand()
{
    vkEndCommandBuffer(p_copy_command_buffer);

    m_queue.Submit(p_copy_command_buffer);

    m_queue.WaitIdle();
}

void VulkanCore::TransitionImageLayout(VkImage &image, VkFormat format, VkImageLayout old_layout,
                                       VkImageLayout new_layout, u32 layer_count, u32 mip_levels)
{
    BeginCommandBuffer(p_copy_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    TransitionImageLayoutCmd(p_copy_command_buffer, image, format, old_layout, new_layout, layer_count, mip_levels);

    SubmitCopyCommand();
}

void VulkanCore::TransitionImageLayoutCmd(VkCommandBuffer command_buffer_ptr, VkImage image, VkFormat format,
                                          VkImageLayout old_layout, VkImageLayout new_layout, u32 layer_count,
                                          u32 mip_levels)
{
    VkImageMemoryBarrier barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                    .pNext = nullptr,
                                    .srcAccessMask = 0,
                                    .dstAccessMask = 0,
                                    .oldLayout = old_layout,
                                    .newLayout = new_layout,
                                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    .image = image,
                                    .subresourceRange = VkImageSubresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                                                .baseMipLevel = 0,
                                                                                .levelCount = mip_levels,
                                                                                .baseArrayLayer = 0,
                                                                                .layerCount = layer_count}};

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

    vkCmdPipelineBarrier(command_buffer_ptr, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanCore::DestroySwapchain() { vkDestroySwapchainKHR(p_device->GetDevice(), p_swap_chain, nullptr); }

void VulkanCore::DestroySwapchainImageViews()
{
    for (size_t i = 0; i < m_image_views.size(); i++) {
        vkDestroyImageView(p_device->GetDevice(), m_image_views[i], nullptr);
    }

    m_image_views.clear();
}

void VulkanCore::ReCreateSwapchain()
{
    DestroySwapchainImageViews();

    VkSurfaceCapabilitiesKHR surface_capabiltities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device->GetPhysicalDevice(), p_instance->GetSurface(),
                                              &surface_capabiltities);

    ENGINE_LOG_DEBUG("Surface capabilities: currentExtent = {}x{}, minImageExtent = {}x{}, maxImageExtent = {}x{}",
                     surface_capabiltities.currentExtent.width, surface_capabiltities.currentExtent.height,
                     surface_capabiltities.minImageExtent.width, surface_capabiltities.minImageExtent.height,
                     surface_capabiltities.maxImageExtent.width, surface_capabiltities.maxImageExtent.height);

    const u32 number_of_images = ChooseNumImages(surface_capabiltities);
    const std::vector<VkPresentModeKHR> &present_modes = p_device->GetSelectedPhysicalDevice().m_present_modes;
    VkPresentModeKHR present_mode = ChoosePresentMode(present_modes, m_vsync_mode);

    m_swap_chain_surface_format =
        ChooseSurfaceFormatAndColorSpace(p_device->GetSelectedPhysicalDevice().m_surface_formats);

    FrameBufferSize framebuffer_size{0, 0};
    GetFramebufferSize(framebuffer_size.width, framebuffer_size.height);

    VkExtent2D extent{0, 0};
    if (surface_capabiltities.currentExtent.width != UINT32_MAX) {
        extent = surface_capabiltities.currentExtent;
    }
    else {
        extent.width =
            std::max(surface_capabiltities.minImageExtent.width,
                     std::min(surface_capabiltities.maxImageExtent.width, static_cast<u32>(framebuffer_size.width)));
        extent.height =
            std::max(surface_capabiltities.minImageExtent.height,
                     std::min(surface_capabiltities.maxImageExtent.height, static_cast<u32>(framebuffer_size.height)));
    }

    ENGINE_LOG_DEBUG("Selected swapchain extent: {}x{}", extent.width, extent.height);

    u32 queue_family_index{p_device->GetQueueFamily()};

    VkSwapchainCreateInfoKHR swap_chain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = p_instance->GetSurface(),
        .minImageCount = number_of_images,
        .imageFormat = m_swap_chain_surface_format.format,
        .imageColorSpace = m_swap_chain_surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT),
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queue_family_index,
        .preTransform = surface_capabiltities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = p_swap_chain // Pass the old swapchain
    };

    VkSwapchainKHR new_swap_chain{VK_NULL_HANDLE};
    ENGINE_LOG_DEBUG("Old swapchain handle: {}", reinterpret_cast<void *>(p_swap_chain));
    VkResult result{vkCreateSwapchainKHR(p_device->GetDevice(), &swap_chain_create_info, nullptr, &new_swap_chain)};
    CHECK_VK_RESULT(result, "vkCreateSwapchainKHR\n");

    // Destroy the old swapchain after the new one is created
    if (p_swap_chain != VK_NULL_HANDLE) {
        ENGINE_LOG_DEBUG("Destroying old swapchain: {}", reinterpret_cast<void *>(p_swap_chain));
        vkDestroySwapchainKHR(p_device->GetDevice(), p_swap_chain, nullptr);
    }
    p_swap_chain = new_swap_chain;
    ENGINE_LOG_DEBUG("New swapchain handle: {}", reinterpret_cast<void *>(p_swap_chain));

    ENGINE_LOG_INFO("Swap chain recreated with V-Sync mode: {}", (m_vsync_mode == VSyncMode::ENABLED) ? "ENABLED (FIFO)"
                                                                 : (m_vsync_mode == VSyncMode::MAILBOX)
                                                                     ? "DISABLED (MAILBOX)"
                                                                     : "DISABLED (IMMEDIATE)");

    CreateSwapchainImageViews();
}

} // end GoudaVK namespace
