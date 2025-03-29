#pragma once

#include <cstdint>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "renderers/vulkan/vk_mesh.hpp"

namespace gouda::vk {

class VulkanRenderer;
class Mesh;

class GraphicsPipeline {
public:
    GraphicsPipeline(VulkanRenderer &renderer, GLFWwindow *window_ptr, VkRenderPass render_pass_ptr,
                     VkShaderModule vertex_shader_ptr, VkShaderModule fragment_shader_ptr, const Mesh *mesh_ptr,
                     int number_of_images, std::vector<Buffer> &uniform_buffers, int uniform_data_size);

    ~GraphicsPipeline();

    void Bind(VkCommandBuffer command_buffer_ptr, size_t image_index) const;
    void Destroy();

private:
    void CreateDescriptorPool(int number_of_images);
    void CreateDescriptorSets(const Mesh *mesh_ptr, int number_of_images, std::vector<Buffer> &uniform_buffers,
                              int uniform_data_size);
    void CreateDescriptorSetLayout(std::vector<Buffer> &uniform_buffers, int uniform_data_size,
                                   VulkanTexture *texture_ptr);
    void AllocateDescriptorSets(int number_of_images);
    void UpdateDescriptorSets(const Mesh *mesh_ptr, int number_of_images, std::vector<Buffer> &uniform_buffers,
                              int uniform_data_size);

private:
    VulkanRenderer &m_renderer;
    VkDevice p_device;
    VkPipeline p_pipeline;
    VkPipelineLayout p_pipeline_layout;

    VkDescriptorPool p_descriptor_pool;
    VkDescriptorSetLayout p_descriptor_set_layout;
    std::vector<VkDescriptorSet> m_descriptor_sets;
};

} // namespace gouda::vk