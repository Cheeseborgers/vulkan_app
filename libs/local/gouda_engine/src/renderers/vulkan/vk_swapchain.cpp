/**
 * @file vk_swapchain.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-19
 * @brief Engine module implementation
 */
#include "renderers/vulkan/vk_swapchain.hpp"

#include <GLFW/glfw3.h>

#include "debug/logger.hpp"
#include "debug/throw.hpp"
#include "math/math.hpp"
#include "renderers/vulkan/vk_buffer_manager.hpp"
#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_instance.hpp"

namespace gouda::vk {

static VkPresentModeKHR GetVulkanPresentMode(VSyncMode mode)
{
    switch (mode) {
        case VSyncMode::Disabled:
            return VK_PRESENT_MODE_IMMEDIATE_KHR; // No sync, high FPS
        case VSyncMode::Enabled:
            return VK_PRESENT_MODE_FIFO_KHR; // Standard V-Sync
        case VSyncMode::Mailbox:
            return VK_PRESENT_MODE_MAILBOX_KHR; // Adaptive V-Sync
        default:
            return VK_PRESENT_MODE_FIFO_KHR;
    }
}

static VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> &present_modes, VSyncMode vsync_mode)
{
    if (vsync_mode == VSyncMode::Enabled) {
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

static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, VkExtent2D extent)
{
    if (capabilities.currentExtent.width != Constants::u32_max) {
        return capabilities.currentExtent; // Vulkan mandates this
    }

    VkExtent2D result_extent{
        math::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        math::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};

    return result_extent;
}
// Implementation -------------------------------------------------------------------------------------
Swapchain::Swapchain(GLFWwindow *window, Device *device, Instance *instance, BufferManager *buffer_manager,
                     VSyncMode vsync_mode)
    : p_swap_chain{VK_NULL_HANDLE},
      m_surface_format{VK_FORMAT_UNDEFINED},
      m_vsync_mode{vsync_mode},
      m_queue_family{0},
      p_window{window},
      p_device{device},
      p_instance{instance},
      p_buffer_manager{buffer_manager},
      m_extent{0, 0}
{
    m_is_valid.store(false, std::memory_order_release);

    UpdateExtent();
    m_queue_family = p_device->GetQueueFamily();
    CreateSwapchain();
    CreateSwapchainImageViews();

    m_is_valid.store(true, std::memory_order_release);
}

Swapchain::~Swapchain()
{
    m_is_valid.store(false, std::memory_order_release);
    DestroySwapchainImageViews();
    DestroySwapchain();
}

void Swapchain::CreateSwapchain()
{
    const VkSurfaceCapabilitiesKHR &surface_capabiltities{p_device->GetSelectedPhysicalDevice().m_surface_capabilities};

    const u32 number_of_images{ChooseNumImages(surface_capabiltities)};

    const std::vector<VkPresentModeKHR> &present_modes{p_device->GetSelectedPhysicalDevice().m_present_modes};
    VkPresentModeKHR present_mode{ChoosePresentMode(present_modes, m_vsync_mode)};

    m_surface_format = ChooseSurfaceFormatAndColorSpace(p_device->GetSelectedPhysicalDevice().m_surface_formats);

    // Log the capabilities for debugging
    ENGINE_LOG_DEBUG("Surface capabilities: currentExtent = {}x{}, minImageExtent = {}x{}, maxImageExtent = {}x{}.",
                     surface_capabiltities.currentExtent.width, surface_capabiltities.currentExtent.height,
                     surface_capabiltities.minImageExtent.width, surface_capabiltities.minImageExtent.height,
                     surface_capabiltities.maxImageExtent.width, surface_capabiltities.maxImageExtent.height);

    // Choose an appropriate extent
    VkExtent2D extent{ChooseSwapExtent(surface_capabiltities, m_extent)};

    ENGINE_LOG_DEBUG("Selected swapchain extent: {}x{}.", extent.width, extent.height);

    VkSwapchainCreateInfoKHR swap_chain_create_info{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = p_instance->GetSurface(),
        .minImageCount = number_of_images,
        .imageFormat = m_surface_format.format,
        .imageColorSpace = m_surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT),
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &m_queue_family,
        .preTransform = surface_capabiltities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = nullptr};

    VkResult result{vkCreateSwapchainKHR(p_device->GetDevice(), &swap_chain_create_info, nullptr, &p_swap_chain)};
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateSwapchainKHR");
    }

    m_is_valid.store(true, std::memory_order_release);

    ENGINE_LOG_DEBUG("Swap chain created with V-Sync mode: {}.", (m_vsync_mode == VSyncMode::Enabled) ? "Enabled (FIFO)"
                                                                 : (m_vsync_mode == VSyncMode::Mailbox)
                                                                     ? "DISABLED (MAILBOX)"
                                                                     : "DISABLED (IMMEDIATE)");
}

void Swapchain::CreateSwapchainImageViews()
{
    u32 number_of_swap_chain_images{0};
    VkResult result{
        vkGetSwapchainImagesKHR(p_device->GetDevice(), p_swap_chain, &number_of_swap_chain_images, nullptr)};

    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkGetSwapchainImagesKHR");
    }

    const VkSurfaceCapabilitiesKHR &surface_capabiltities{p_device->GetSelectedPhysicalDevice().m_surface_capabilities};

    const u32 number_of_images{ChooseNumImages(surface_capabiltities)};

    ASSERT(number_of_images == number_of_swap_chain_images,
           "Number of images is not equal to the number of swap chain images.");

    ENGINE_LOG_DEBUG("Created {} swap chain images.", number_of_swap_chain_images);

    m_images.resize(number_of_swap_chain_images);
    m_image_views.resize(number_of_swap_chain_images);

    result =
        vkGetSwapchainImagesKHR(p_device->GetDevice(), p_swap_chain, &number_of_swap_chain_images, m_images.data());
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkGetSwapchainImagesKHR\n");
    }

    u32 layer_count{1};
    u32 mip_levels{1};
    for (u32 i = 0; i < number_of_swap_chain_images; i++) {
        m_image_views[i] =
            p_buffer_manager->CreateImageView(m_images[i], m_surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT,
                                              VK_IMAGE_VIEW_TYPE_2D, layer_count, mip_levels);
    }
}

void Swapchain::DestroySwapchain()
{
    if (p_swap_chain) {
        m_is_valid.store(false, std::memory_order_release);

        vkDestroySwapchainKHR(p_device->GetDevice(), p_swap_chain, nullptr);
        p_swap_chain = VK_NULL_HANDLE;
    }
}

void Swapchain::DestroySwapchainImageViews()
{
    if (p_swap_chain) {
        for (auto image_view : m_image_views) {
            vkDestroyImageView(p_device->GetDevice(), image_view, nullptr);
            image_view = VK_NULL_HANDLE;
        }

        m_image_views.clear();
    }
    ENGINE_LOG_DEBUG("Swap chain images destroyed.");
}

void Swapchain::Recreate()
{
    m_is_valid.store(false, std::memory_order_release);

    UpdateExtent();

    DestroySwapchainImageViews();

    VkSurfaceCapabilitiesKHR surface_capabiltities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device->GetPhysicalDevice(), p_instance->GetSurface(),
                                              &surface_capabiltities);

    ENGINE_LOG_DEBUG("Surface capabilities: currentExtent = {}x{}, minImageExtent = {}x{}, maxImageExtent = {}x{}.",
                     surface_capabiltities.currentExtent.width, surface_capabiltities.currentExtent.height,
                     surface_capabiltities.minImageExtent.width, surface_capabiltities.minImageExtent.height,
                     surface_capabiltities.maxImageExtent.width, surface_capabiltities.maxImageExtent.height);

    const u32 number_of_images{ChooseNumImages(surface_capabiltities)};
    const std::vector<VkPresentModeKHR> &present_modes{p_device->GetSelectedPhysicalDevice().m_present_modes};
    VkPresentModeKHR present_mode{ChoosePresentMode(present_modes, m_vsync_mode)};

    m_surface_format = ChooseSurfaceFormatAndColorSpace(p_device->GetSelectedPhysicalDevice().m_surface_formats);

    UpdateExtent();

    VkExtent2D extent{0, 0};
    if (surface_capabiltities.currentExtent.width != math::max_value<u32>()) {
        extent = surface_capabiltities.currentExtent;
    }
    else {
        extent.width = math::max(surface_capabiltities.minImageExtent.width,
                                 math::min(surface_capabiltities.maxImageExtent.width, m_extent.width));
        extent.height = math::max(surface_capabiltities.minImageExtent.height,
                                  math::min(surface_capabiltities.maxImageExtent.height, m_extent.height));
    }

    ENGINE_LOG_DEBUG("Selected swapchain extent: {}x{}.", extent.width, extent.height);

    VkSwapchainCreateInfoKHR swap_chain_create_info{};
    swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_chain_create_info.pNext = nullptr;
    swap_chain_create_info.flags = 0;
    swap_chain_create_info.surface = p_instance->GetSurface();
    swap_chain_create_info.minImageCount = number_of_images;
    swap_chain_create_info.imageFormat = m_surface_format.format;
    swap_chain_create_info.imageColorSpace = m_surface_format.colorSpace;
    swap_chain_create_info.imageExtent = extent;
    swap_chain_create_info.imageArrayLayers = 1;
    swap_chain_create_info.imageUsage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_chain_create_info.queueFamilyIndexCount = 1;
    swap_chain_create_info.pQueueFamilyIndices = &m_queue_family;
    swap_chain_create_info.preTransform = surface_capabiltities.currentTransform;
    swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_chain_create_info.presentMode = present_mode;
    swap_chain_create_info.clipped = VK_TRUE;
    swap_chain_create_info.oldSwapchain = p_swap_chain; // Pass the old swapchain

    VkSwapchainKHR new_swap_chain{VK_NULL_HANDLE};
    ENGINE_LOG_DEBUG("Old swapchain handle: {}.", reinterpret_cast<void *>(p_swap_chain));
    VkResult result{vkCreateSwapchainKHR(p_device->GetDevice(), &swap_chain_create_info, nullptr, &new_swap_chain)};
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateSwapchainKHR\n");
    }

    // Destroy the old swapchain after the new one is created
    if (p_swap_chain != VK_NULL_HANDLE) {
        ENGINE_LOG_DEBUG("Destroying old swapchain: {}.", reinterpret_cast<void *>(p_swap_chain));
        vkDestroySwapchainKHR(p_device->GetDevice(), p_swap_chain, nullptr);
    }
    p_swap_chain = new_swap_chain;
    ENGINE_LOG_DEBUG("New swapchain handle: {}.", reinterpret_cast<void *>(p_swap_chain));

    ENGINE_LOG_DEBUG("Swap chain recreated with V-Sync mode: {}.",
                     (m_vsync_mode == VSyncMode::Enabled)   ? "Enabled (FIFO)"
                     : (m_vsync_mode == VSyncMode::Mailbox) ? "DISABLED (MAILBOX)"
                                                            : "DISABLED (IMMEDIATE)");

    CreateSwapchainImageViews();

    m_is_valid.store(true, std::memory_order_release);
}

void Swapchain::UpdateExtent()
{
    FrameBufferSize size;
    glfwGetWindowSize(p_window, &size.width, &size.height);
    m_extent = {static_cast<u32>(size.width), static_cast<u32>(size.height)};
}

} // namespace gouda::vk
