#pragma once

#include <string_view>

#include <vulkan/vulkan.h>

namespace GoudaVK {

VkShaderModule CreateShaderModuleFromBinary(VkDevice device_ptr, std::string_view file_name);

VkShaderModule CreateShaderModuleFromText(VkDevice device_ptr, std::string_view file_name);

} // end namespace