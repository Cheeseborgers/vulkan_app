#pragma once

/**
 * @file vk_compute_pipeline.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-15
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
#include "renderers/vulkan/vk_buffer.hpp"

namespace gouda::vk {

class Renderer;
class Shader;

class ComputePipeline {
public:
    ComputePipeline(Renderer &renderer, const Shader *compute_shader, const std::vector<Buffer> &storage_buffers,
                    const std::vector<Buffer> &uniform_buffers, VkDeviceSize storage_buffer_size,
                    VkDeviceSize uniform_buffer_size);
    ~ComputePipeline();

    void Bind(VkCommandBuffer command_buffer) const;
    void BindDescriptors(VkCommandBuffer command_buffer, u32 image_index) const;
    u32 CalculateWorkGroupCount(u32 particle_count) const;
    void Dispatch(VkCommandBuffer command_buffer, u32 group_count_x, u32 group_count_y, u32 group_count_z) const;
    void Destroy();

    VkPipeline GetPipeline() const { return p_pipeline; }
    VkPipelineLayout GetPipelineLayout() const { return p_pipeline_layout; }

private:
    Renderer &m_renderer;
    VkDevice p_device;
    VkPipeline p_pipeline{VK_NULL_HANDLE};
    VkPipelineLayout p_pipeline_layout{VK_NULL_HANDLE};
    VkDescriptorSetLayout p_descriptor_set_layout{VK_NULL_HANDLE};
    std::vector<VkDescriptorSet> m_descriptor_sets;
    VkDescriptorPool p_descriptor_pool{VK_NULL_HANDLE};
};

} // namespace gouda::vk