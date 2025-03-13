#pragma once

#include <string_view>

#include <vulkan/vulkan.h>

namespace gouda {
namespace vk {

VkShaderModule CreateShaderModuleFromBinary(VkDevice device_ptr, std::string_view file_name);

VkShaderModule CreateShaderModuleFromText(VkDevice device_ptr, std::string_view file_name);

} // namesapce vk
} // namespace gouda