#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "core/types.hpp"
#include "gouda_vk_instance.hpp"

namespace GoudaVK {

struct PhysicalDevice {
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties m_device_properties{};
    std::vector<VkQueueFamilyProperties> m_queue_family_properties;
    std::vector<VkBool32> m_queue_supports_present;
    std::vector<VkSurfaceFormatKHR> m_surface_formats;
    VkSurfaceCapabilitiesKHR m_surface_capabilties{};
    VkPhysicalDeviceMemoryProperties m_memory_properties{};
    std::vector<VkPresentModeKHR> m_present_modes;
    VkPhysicalDeviceFeatures m_features{};
    VkFormat m_depth_format = VK_FORMAT_UNDEFINED;
};

class VulkanPhysicalDevices {
public:
    VulkanPhysicalDevices() : m_dev_index{-1} {}

    void Init(const VulkanInstance &instance, const VkSurfaceKHR &surface);
    u32 SelectDevice(VkQueueFlags required_queue_type, bool supports_present);
    const PhysicalDevice &Selected() const;

private:
    std::vector<PhysicalDevice> m_devices;
    int m_dev_index;
};

class VulkanDevice {
public:
    VulkanDevice(const VulkanInstance &instance, VkQueueFlags required_queue_flags);
    ~VulkanDevice();

    VkDevice GetDevice() const { return p_device; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_physical_devices.Selected().m_physical_device; }
    u32 GetQueueFamily() const { return m_queue_family; }
    const PhysicalDevice &GetSelectedPhysicalDevice() const { return m_physical_devices.Selected(); }

    void Wait() { vkDeviceWaitIdle(p_device); };

private:
    VkDevice p_device;
    VulkanPhysicalDevices m_physical_devices;
    u32 m_queue_family;

    void CreateDevice(VkQueueFlags required_queue_flags);
};

} // end namespace