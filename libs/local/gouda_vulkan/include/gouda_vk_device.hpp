#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "core/types.hpp"
#include "gouda_vk_instance.hpp"

namespace gouda {
namespace vk {

struct PhysicalDevice {
    VkPhysicalDevice m_physical_device;
    VkPhysicalDeviceProperties m_device_properties;
    VkPhysicalDeviceMemoryProperties m_memory_properties;
    VkPhysicalDeviceFeatures m_features;
    VkSurfaceCapabilitiesKHR m_surface_capabilities;
    VkFormat m_depth_format;

    std::vector<VkQueueFamilyProperties> m_queue_family_properties;
    std::vector<VkBool32> m_queue_supports_present;
    std::vector<VkSurfaceFormatKHR> m_surface_formats;
    std::vector<VkPresentModeKHR> m_present_modes;

    PhysicalDevice()
        : m_physical_device{VK_NULL_HANDLE},
          m_device_properties{},
          m_memory_properties{},
          m_features{},
          m_surface_capabilities{},
          m_depth_format{VK_FORMAT_UNDEFINED},
          m_queue_family_properties{},
          m_queue_supports_present{},
          m_surface_formats{},
          m_present_modes{}
    {
    }
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

} // namespace vk
} // namespace gouda