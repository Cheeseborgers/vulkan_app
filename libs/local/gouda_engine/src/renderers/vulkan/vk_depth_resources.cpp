/**
 * @file vk_depth_resources.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-19
 * @brief Engine module implementation
 */
#include "renderers/vulkan/vk_depth_resources.hpp"

#include "debug/assert.hpp"
#include "debug/logger.hpp"
#include "renderers/vulkan/vk_buffer_manager.hpp"
#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_swapchain.hpp"

namespace gouda::vk {

DepthResources::DepthResources(Device *device, Instance *instance, BufferManager *buffer_manager, Swapchain *swapchain)
    : p_swapchain{swapchain}, p_device{device}, p_instance{instance}, p_buffer_manager{buffer_manager}
{
    Create();
}

DepthResources::~DepthResources() { Destroy(); }

void DepthResources::Create()
{
    ASSERT(p_device, "Device cannot be a null pointer.");
    ASSERT(p_instance, "Instance cannot be a null pointer.");
    ASSERT(p_buffer_manager, "Buffer manager cannot be a null pointer.");
    ASSERT(p_swapchain, "Swapchain cannot be a null pointer.");

    const int swapchain_image_count{static_cast<int>(p_swapchain->GetImages().size())};

    const VkFormat depth_format{p_device->GetSelectedPhysicalDevice().m_depth_format};
    ASSERT(depth_format != VK_FORMAT_UNDEFINED, "Depth format is not set properly!");

    ImageSize image_size{p_swapchain->GetFramebufferSize()};
    ASSERT(image_size.width > 0 && image_size.height > 0, "Framebuffer size must be greater than 0!");

    // Resize depth images vector to match swapchain image count
    m_depth_images.resize(swapchain_image_count);

    // Create depth images and associated resources
    for (auto &depth_image : m_depth_images) {

        // Create the depth image for each swapchain image
        p_buffer_manager->CreateDepthImage(depth_image, image_size, depth_format);
        p_buffer_manager->TransitionImageLayout(depth_image.p_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED,
                                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);

        depth_image.p_view = p_buffer_manager->CreateImageView(depth_image.p_image, depth_format,
                                                               VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, 1);
    }

    ENGINE_LOG_DEBUG("Depth resources created of size: {}x{}.", image_size.width, image_size.height);
}

void DepthResources::Recreate()
{
    Destroy();
    Create();
}

void DepthResources::Destroy()
{
    for (auto depth_image : m_depth_images) {
        depth_image.Destroy(p_device);
    }

    m_depth_images.clear();

    ENGINE_LOG_DEBUG("Depth resources destroyed.");
}

} // namespace gouda::vk
