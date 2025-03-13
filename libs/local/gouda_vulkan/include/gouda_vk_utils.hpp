#pragma once

#include <format>
#include <iostream>
#include <stdexcept>

#include "core/types.hpp"

namespace gouda {
namespace vk {

inline void check_vk_result(VkResult res, const char *msg, const char *file, int line)
{
    if (res != VK_SUCCESS) {
        std::cerr << std::format("Error in {}:{} - {}, code {:#x}\n", file, line, msg, static_cast<int>(res));
        throw std::runtime_error("Vulkan operation failed.");
    }
}

#define CHECK_VK_RESULT(res, msg) check_vk_result(res, msg, __FILE__, __LINE__)

void PrintVKExtensions();

const char *GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT Severity);

const char *GetDebugType(VkDebugUtilsMessageTypeFlagsEXT Type);

VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features);

} // namesapce vk
} // namespace gouda