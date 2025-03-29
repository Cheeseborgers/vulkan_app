/**
 * @file vk_shader_manager.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-28
 * @brief Engine module implementation
 */
#include "renderers/vulkan/vk_shader_manager.hpp"

#include "renderers/vulkan/vk_shader.hpp"

namespace gouda::vk {

ShaderManager::ShaderManager(VkDevice device) : p_device(device) {}

ShaderManager::~ShaderManager() { Cleanup(); }

VkShaderModule ShaderManager::GetShader(std::string_view filename)
{
    // Check if shader is cached
    if (m_shader_cache.contains(filename.data())) {
        return m_shader_cache[filename.data()];
    }

    // Load shader if not in cache
    VkShaderModule shader_module = CreateShaderModuleFromBinary(p_device, filename);
    m_shader_cache[filename.data()] = shader_module;
    return shader_module;
}

void ShaderManager::Cleanup()
{
    for (auto &[filename, shader] : m_shader_cache) {
        vkDestroyShaderModule(p_device, shader, nullptr);
    }
    m_shader_cache.clear();
}

void ShaderManager::WatchForChanges() {}

} // namespace gouda::vk