#pragma once

#include <cstdint>
#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "gouda_vk_simple_mesh.hpp"

namespace gouda {
namespace vk {

class GraphicsPipeline {
public:
    GraphicsPipeline(VkDevice device_ptr, GLFWwindow *window_ptr, VkRenderPass render_pass_ptr,
                     VkShaderModule vertex_shader_ptr, VkShaderModule fragment_shader_ptr, const SimpleMesh *mesh_ptr,
                     int number_of_images, std::vector<AllocatedBuffer> &uniform_buffers, int uniform_data_size);

    ~GraphicsPipeline();

    void Bind(VkCommandBuffer command_buffer_ptr, std::size_t image_index);
    void Destroy();

private:
    void CreateDescriptorPool(int number_of_images);
    void CreateDescriptorSets(const SimpleMesh *mesh_ptr, int number_of_images,
                              std::vector<AllocatedBuffer> &uniform_buffers, int uniform_data_size);
    void CreateDescriptorSetLayout(std::vector<AllocatedBuffer> &uniform_buffers, int uniform_data_size,
                                   VulkanTexture *texture_ptr);
    void AllocateDescriptorSets(int number_of_images);
    void UpdateDescriptorSets(const SimpleMesh *mesh_ptr, int number_of_images,
                              std::vector<AllocatedBuffer> &uniform_buffers, int uniform_data_size);

private:
    VkDevice p_device;
    VkPipeline p_pipeline;
    VkPipelineLayout p_pipeline_layout;

    VkDescriptorPool p_descriptor_pool;
    VkDescriptorSetLayout p_descriptor_set_layout;
    std::vector<VkDescriptorSet> m_descriptor_sets;
};

} // namespace vk
} // namespace gouda