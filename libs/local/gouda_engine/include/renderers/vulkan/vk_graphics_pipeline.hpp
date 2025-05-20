#pragma once
/**
 * @file vk_graphics_pipeline.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-15
 * @brief Engine module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <vector>

#include <vulkan/vulkan.h>

#include "containers/small_vector.hpp"
#include "core/types.hpp"

struct GLFWwindow;

namespace gouda::vk {

class Buffer;
class Renderer;
class Shader;
class Texture;

enum class PipelineType : u8 { Quad, Text, Particle };

class GraphicsPipeline {
public:
    GraphicsPipeline(Renderer &renderer, VkRenderPass render_pass, Shader *vertex_shader,
                     Shader *fragment_shader, int number_of_images, Vector<Buffer> &uniform_buffers,
                     int uniform_data_size, PipelineType type);

    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline &) = delete;
    GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;
    GraphicsPipeline(GraphicsPipeline &&) = default;
    GraphicsPipeline &operator=(GraphicsPipeline &&) = delete;

    void Bind(VkCommandBuffer command_buffer_ptr, size_t image_index) const;
    void Destroy();

    void UpdateTextureDescriptors(size_t number_of_images, const Vector<std::unique_ptr<Texture>> &textures);
    void UpdateFontTextureDescriptors(size_t number_of_images, const Vector<std::unique_ptr<Texture>> &font_textures);

    [[nodiscard]] constexpr VkPipeline GetPipeline() const noexcept { return p_pipeline; }
    [[nodiscard]] constexpr VkPipelineLayout GetLayout() const noexcept { return p_pipeline_layout; }

private:
    void CreateDescriptorPool(int number_of_images);
    void CreateDescriptorSets(int number_of_images, const Vector<Buffer> &uniform_buffers, int uniform_data_size);
    void CreateDescriptorSetLayout();
    void AllocateDescriptorSets(int number_of_images);
    void UpdateDescriptorSets(int number_of_images, const Vector<Buffer> &uniform_buffers, int uniform_data_size);

    [[nodiscard]] std::array<VkPipelineShaderStageCreateInfo, 2> SetupShaderStages() const;
    [[nodiscard]] VkPipelineVertexInputStateCreateInfo SetupVertexInput();

    /**
     * @brief Sets up pipeline states (input assembly, viewport, rasterization, etc.).
     * @return Struct containing pipeline state create infos.
     */
    struct PipelineStates {
        VkPipelineInputAssemblyStateCreateInfo input_assembly;
        VkPipelineViewportStateCreateInfo viewport;
        VkPipelineRasterizationStateCreateInfo rasterization;
        VkPipelineMultisampleStateCreateInfo multisample;
        VkPipelineDepthStencilStateCreateInfo depth_stencil;
        VkPipelineColorBlendStateCreateInfo color_blend;
        VkPipelineDynamicStateCreateInfo dynamic;
        VkPipelineColorBlendAttachmentState blend_attachment;
    };
    [[nodiscard]] PipelineStates SetupPipelineStates() const;
    [[nodiscard]] Vector<VkPushConstantRange> SetupPushConstants();

    void PrintReflection() const;

private:
    Renderer &m_renderer;
    VkDevice p_device;
    VkPipeline p_pipeline;
    VkPipelineLayout p_pipeline_layout;

    VkDescriptorPool p_descriptor_pool;
    Vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
    Vector<Vector<VkDescriptorSet>> m_descriptor_sets;
    Vector<VkVertexInputBindingDescription> m_binding_descriptions;
    Vector<VkVertexInputAttributeDescription> m_attribute_descriptions;

    PipelineType m_type;

    Shader *p_vertex_shader;
    Shader *p_fragment_shader;
};

} // namespace gouda::vk