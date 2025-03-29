#pragma once
/**
 * @file vk_shader_manager.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-28
 * @brief Engine module
 *
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

namespace gouda::vk {

class ShaderManager {
public:
    ShaderManager(VkDevice device);
    ~ShaderManager();

    // Load and retrieve shader modules
    VkShaderModule GetShader(std::string_view filename);

    // Clean up shaders
    void Cleanup();

    // Optional: Shader hot reloading
    void WatchForChanges();

private:
    VkDevice p_device;
    std::unordered_map<std::string, VkShaderModule> m_shader_cache;
};

} // namespace gouda::vk