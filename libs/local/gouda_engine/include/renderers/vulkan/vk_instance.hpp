#pragma once

/**
 * @file vk_instance.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-23
 * @brief Engine module
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <string_view>
#include <vector>

#include <vulkan/vulkan.h>

#include "vk_utils.hpp"

struct GLFWwindow;

namespace gouda::vk {

class Instance {
public:
    Instance(std::string_view app_name, SemVer vulkan_api_version, GLFWwindow *window);
    ~Instance();

    VkInstance GetInstance() const { return p_instance; }
    VkSurfaceKHR GetSurface() const { return p_surface; }

private:
    void CreateInstance(std::string_view app_name, SemVer vulkan_api_version);
    void CreateDebugCallback();
    void CreateSurface();

private:
    VkInstance p_instance;
    VkDebugUtilsMessengerEXT p_debug_messenger;
    VkSurfaceKHR p_surface;
    GLFWwindow *p_window;
};

} // namespace gouda::vk