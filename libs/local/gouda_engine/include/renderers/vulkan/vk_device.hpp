#pragma once
/**
 * @file vk_device.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-24
 * @brief Engine module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <vector>

#include <vulkan/vulkan.h>

#include "core/types.hpp"
#include "renderers/vulkan/vk_instance.hpp"
#include "containers/small_vector.hpp"

namespace gouda::vk {

constexpr u32 MAX_TEXTURES{32};

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
          m_depth_format{VK_FORMAT_UNDEFINED}
    {
    }
};

class VulkanPhysicalDevices {
public:
    VulkanPhysicalDevices() : m_dev_index{-1} {}

    void Initialize(const Instance &instance, const VkSurfaceKHR &surface);
    u32 SelectDevice(VkQueueFlags required_queue_type, bool supports_present);
    [[nodiscard]] const PhysicalDevice &Selected() const;

private:
    std::vector<PhysicalDevice> m_devices;
    int m_dev_index;
};

class Device {
public:
    Device(const Instance &instance, VkQueueFlags required_queue_flags);
    ~Device();

    [[nodiscard]] VkDevice GetDevice() const { return p_device; }
    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_physical_devices.Selected().m_physical_device; }
    [[nodiscard]] u32 GetQueueFamily() const { return m_queue_family; }
    [[nodiscard]] const PhysicalDevice &GetSelectedPhysicalDevice() const { return m_physical_devices.Selected(); }

    void Wait() const { vkDeviceWaitIdle(p_device); };

private:
    void CreateDevice(VkQueueFlags required_queue_flags);

private:
    VkDevice p_device;
    VulkanPhysicalDevices m_physical_devices;
    u32 m_queue_family;
};

} // namespace gouda::vk