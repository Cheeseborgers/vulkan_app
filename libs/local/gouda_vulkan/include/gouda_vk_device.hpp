#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "gouda_types.hpp"

namespace GoudaVK {

struct PhysicalDevice {
    VkPhysicalDevice m_phys_device;
    VkPhysicalDeviceProperties m_dev_props;
    std::vector<VkQueueFamilyProperties> m_queue_family_props;
    std::vector<VkBool32> m_queue_supports_present;
    std::vector<VkSurfaceFormatKHR> m_surface_formats;
    VkSurfaceCapabilitiesKHR m_surface_capabilties;
    VkPhysicalDeviceMemoryProperties m_memory_props;
    std::vector<VkPresentModeKHR> m_present_modes;
    VkPhysicalDeviceFeatures m_features;
    VkFormat m_depth_format;
};

class VulkanPhysicalDevices {
public:
    VulkanPhysicalDevices() : m_dev_index{-1} {}
    ~VulkanPhysicalDevices() {}

    void Init(const VkInstance &instance, const VkSurfaceKHR &surface);

    u32 SelectDevice(VkQueueFlags required_queue_type, bool supports_present);

    const PhysicalDevice &Selected() const;

private:
    std::vector<PhysicalDevice> m_devices;

    int m_dev_index;
};

} // end namespace