#pragma once

#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "gouda_vk_utils.hpp"

namespace GoudaVK {
class VKInstance {
public:
    VKInstance(std::string_view app_name, SemVer vulkan_api_version, GLFWwindow *window);
    ~VKInstance();

    VkInstance GetInstance() const { return p_instance; }
    VkSurfaceKHR GetSurface() const { return p_surface; }

private:
    VkInstance p_instance;
    VkDebugUtilsMessengerEXT p_debug_messenger;
    VkSurfaceKHR p_surface;
    GLFWwindow *p_window;

    void CreateInstance(std::string_view app_name, SemVer vulkan_api_version);
    void CreateDebugCallback();
    void CreateSurface();
};
} // namespace GoudaVK