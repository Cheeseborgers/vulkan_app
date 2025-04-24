#pragma once
/**
 * @file vk_utils.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-24
 * @brief Engine module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "core/types.hpp"
#include "debug/logger.hpp"
#include "debug/throw.hpp"

namespace gouda::vk {

inline void check_vk_result(VkResult res, const char *msg, const char *file, int line)
{
    if (res != VK_SUCCESS) {
        ENGINE_LOG_FATAL("Error in {}:{} - {}, code {:#x}", file, line, msg, static_cast<int>(res));
        ENGINE_THROW("Vulkan operation failed.");
    }
}

template <typename... Args>
inline void check_vk_result(VkResult res, const char *msg, const char *file, int line, Args &&...args)
{
    if (res != VK_SUCCESS) {
        ENGINE_LOG_FATAL("Error in {}:{} - {}, code {:#x} - ", file, line, msg, static_cast<int>(res));
        (ENGINE_LOG_FATAL("{}", std::forward<Args>(args)), ...); // Using fold expression for additional args
        ENGINE_THROW("Vulkan operation failed.");
    }
}

#define CHECK_VK_RESULT(res, msg, ...) check_vk_result(res, msg, __FILE__, __LINE__, ##__VA_ARGS__)

// TODO: Change the case of the two functions bellow
void PrintVKExtensions();

VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features);

std::string_view vk_result_to_string(VkResult result);

} // namespace gouda::vk