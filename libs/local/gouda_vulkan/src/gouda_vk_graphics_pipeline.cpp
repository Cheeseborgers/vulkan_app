#include "gouda_vk_graphics_pipeline.hpp"

#include "logger.hpp"

#include "core/types.hpp"
#include "debug/assert.hpp"

#include "gouda_vk_utils.hpp"

namespace gouda {
namespace vk {

// TODO: Change these pointer names to ..._ptr
GraphicsPipeline::GraphicsPipeline(VkDevice device_ptr, GLFWwindow *p_window, VkRenderPass p_render_pass,
                                   VkShaderModule p_vertex_shader, VkShaderModule p_fragment_shader,
                                   const SimpleMesh *mesh_ptr, int number_of_images,
                                   std::vector<Buffer> &uniform_buffers, int uniform_data_size)
    : p_device{device_ptr},
      p_pipeline{VK_NULL_HANDLE},
      p_pipeline_layout{VK_NULL_HANDLE},
      p_descriptor_pool{VK_NULL_HANDLE},
      p_descriptor_set_layout{VK_NULL_HANDLE}
{
    if (mesh_ptr) {
        CreateDescriptorSets(mesh_ptr, number_of_images, uniform_buffers, uniform_data_size);
    }

    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stage_create_info = {
        VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                                        VK_SHADER_STAGE_VERTEX_BIT, p_vertex_shader, "main", nullptr},
        VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                                        VK_SHADER_STAGE_FRAGMENT_BIT, p_fragment_shader, "main", nullptr}};

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info{};
    pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    // Define dynamic states
    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state_info = {.sType =
                                                               VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                                           .dynamicStateCount = 2,
                                                           .pDynamicStates = dynamic_states};

    // Viewport state with no static values
    VkPipelineViewportStateCreateInfo viewport_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,    // Required even for dynamic
        .pViewports = nullptr, // Set dynamically at runtime
        .scissorCount = 1,
        .pScissors = nullptr // Set dynamically at runtime
    };

    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info{};
    pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
    pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipeline_rasterization_state_create_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo pipeline_multisampler_create_info{};
    pipeline_multisampler_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline_multisampler_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline_multisampler_create_info.sampleShadingEnable = VK_FALSE;
    pipeline_multisampler_create_info.minSampleShading = 1.0f;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_create_info.front = {};
    depth_stencil_state_create_info.back = {};
    depth_stencil_state_create_info.minDepthBounds = 0.0f;
    depth_stencil_state_create_info.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState blend_attach_state{};
    blend_attach_state.blendEnable = VK_FALSE;
    blend_attach_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend_create_info{};
    blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_create_info.logicOpEnable = VK_FALSE;
    blend_create_info.attachmentCount = 1;
    blend_create_info.pAttachments = &blend_attach_state;

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (mesh_ptr && mesh_ptr->m_vertex_buffer.p_buffer) {
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = &p_descriptor_set_layout;
    }
    else {
        layout_info.setLayoutCount = 0;
        layout_info.pSetLayouts = nullptr;
    }

    VkResult result = vkCreatePipelineLayout(p_device, &layout_info, nullptr, &p_pipeline_layout);
    CHECK_VK_RESULT(result, "vkCreatePipelineLayout\n");

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = std::size(shader_stage_create_info);
    pipeline_info.pStages = &shader_stage_create_info[0];
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
    pipeline_info.pViewportState = &viewport_state_create_info;
    pipeline_info.pRasterizationState = &pipeline_rasterization_state_create_info;
    pipeline_info.pMultisampleState = &pipeline_multisampler_create_info;
    pipeline_info.pDepthStencilState = &depth_stencil_state_create_info;
    pipeline_info.pColorBlendState = &blend_create_info;
    pipeline_info.pDynamicState = &dynamic_state_info; // Enable dynamic states
    pipeline_info.layout = p_pipeline_layout;
    pipeline_info.renderPass = p_render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(p_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &p_pipeline);
    CHECK_VK_RESULT(result, "vkCreateGraphicsPipelines\n");

    ENGINE_LOG_DEBUG("Graphics pipeline created");
}

GraphicsPipeline::~GraphicsPipeline() { Destroy(); }

void GraphicsPipeline::Bind(VkCommandBuffer command_buffer_ptr, size_t image_index)
{
    vkCmdBindPipeline(command_buffer_ptr, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline);

    if (m_descriptor_sets.size() > 0) {
        vkCmdBindDescriptorSets(command_buffer_ptr, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline_layout,
                                0, // firstSet
                                1, // descriptorSetCount
                                &m_descriptor_sets[image_index],
                                0,        // dynamicOffsetCount
                                nullptr); // pDynamicOffsets
    }
}

void GraphicsPipeline::Destroy()
{
    if (p_descriptor_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(p_device, p_descriptor_set_layout, nullptr);
        p_descriptor_set_layout = VK_NULL_HANDLE;
    }

    if (p_pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(p_device, p_pipeline_layout, nullptr);
        p_pipeline_layout = VK_NULL_HANDLE;
    }

    if (p_descriptor_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(p_device, p_descriptor_pool, nullptr);
        p_descriptor_pool = VK_NULL_HANDLE;
    }

    if (p_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(p_device, p_pipeline, nullptr);
        p_pipeline = VK_NULL_HANDLE;
    }

    ENGINE_LOG_DEBUG("Graphics pipeline destroyed");
}

void GraphicsPipeline::CreateDescriptorPool(int number_of_images)
{
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         static_cast<u32>(number_of_images)}, // One for each image for the vertex buffer
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         static_cast<u32>(number_of_images)}, // One for each image for the uniform buffer
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         static_cast<u32>(number_of_images)} // One for each image for the combined image sampler
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                              .pNext = nullptr,
                                                              .flags = 0,
                                                              .maxSets = static_cast<u32>(number_of_images),
                                                              .poolSizeCount = 3, // Update poolSizeCount to 3
                                                              .pPoolSizes = pool_sizes};

    VkResult result{vkCreateDescriptorPool(p_device, &descriptor_pool_create_info, nullptr, &p_descriptor_pool)};
    CHECK_VK_RESULT(result, "vkCreateDescriptorPool");

    ENGINE_LOG_DEBUG("Descriptor pool created");
}

void GraphicsPipeline::CreateDescriptorSets(const SimpleMesh *mesh_ptr, int number_of_images,
                                            std::vector<Buffer> &uniform_buffers, int uniform_data_size)
{
    CreateDescriptorPool(number_of_images);

    CreateDescriptorSetLayout(uniform_buffers, uniform_data_size, mesh_ptr->p_texture);

    AllocateDescriptorSets(number_of_images);

    UpdateDescriptorSets(mesh_ptr, number_of_images, uniform_buffers, uniform_data_size);
}

void GraphicsPipeline::CreateDescriptorSetLayout(std::vector<Buffer> &uniform_buffers, int uniform_data_size,
                                                 VulkanTexture *texture_ptr)
{
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;

    // Vertex buffer
    VkDescriptorSetLayoutBinding vertex_shader_layout_binding_vertex_buffer{};
    vertex_shader_layout_binding_vertex_buffer.binding = 0;
    vertex_shader_layout_binding_vertex_buffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vertex_shader_layout_binding_vertex_buffer.descriptorCount = 1;
    vertex_shader_layout_binding_vertex_buffer.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    layout_bindings.push_back(vertex_shader_layout_binding_vertex_buffer);

    // Uniform buffers
    VkDescriptorSetLayoutBinding vertex_shader_layout_binding_uniform_buffer{};
    vertex_shader_layout_binding_uniform_buffer.binding = 1;
    vertex_shader_layout_binding_uniform_buffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vertex_shader_layout_binding_uniform_buffer.descriptorCount = 1;
    vertex_shader_layout_binding_uniform_buffer.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    if (uniform_buffers.size() > 0) {
        layout_bindings.push_back(vertex_shader_layout_binding_uniform_buffer);
    }

    // Texture
    if (texture_ptr) {
        VkDescriptorSetLayoutBinding fragment_shader_layout_binding = {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };

        layout_bindings.push_back(fragment_shader_layout_binding);
    }

    VkDescriptorSetLayoutCreateInfo layout_info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                .pNext = nullptr,
                                                .flags = 0, // reserved - must be zero
                                                .bindingCount = static_cast<u32>(layout_bindings.size()),
                                                .pBindings = layout_bindings.data()};

    VkResult result{vkCreateDescriptorSetLayout(p_device, &layout_info, nullptr, &p_descriptor_set_layout)};
    CHECK_VK_RESULT(result, "vkCreateDescriptorSetLayout");
}

void GraphicsPipeline::AllocateDescriptorSets(int number_of_images)
{
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts(static_cast<size_t>(number_of_images),
                                                              p_descriptor_set_layout);

    VkDescriptorSetAllocateInfo descriptor_set_allocation_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = p_descriptor_pool,
        .descriptorSetCount = static_cast<u32>(number_of_images),
        .pSetLayouts = descriptor_set_layouts.data()};

    m_descriptor_sets.resize(static_cast<size_t>(number_of_images));

    VkResult result{vkAllocateDescriptorSets(p_device, &descriptor_set_allocation_info, m_descriptor_sets.data())};
    CHECK_VK_RESULT(result, "vkAllocateDescriptorSets");
}

void GraphicsPipeline::UpdateDescriptorSets(const SimpleMesh *mesh_ptr, int number_of_images,
                                            std::vector<Buffer> &uniform_buffers, int uniform_data_size)
{
    VkDescriptorBufferInfo descriptor_buffer_info_vertex = {
        .buffer = mesh_ptr->m_vertex_buffer.p_buffer,
        .offset = 0,
        .range = mesh_ptr->m_vertex_buffer_size,
    };

    VkDescriptorImageInfo image_info;

    ASSERT(mesh_ptr, "GraphicsPipeline::UpdateDescriptorSets : Mesh pointer cannot be null");
    ASSERT(mesh_ptr->p_texture, "GraphicsPipeline::UpdateDescriptorSets : Mesh texture pointer cannot be null");

    if (mesh_ptr->p_texture) {
        image_info.sampler = mesh_ptr->p_texture->p_sampler;
        image_info.imageView = mesh_ptr->p_texture->p_view;
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    std::vector<VkWriteDescriptorSet> write_descriptor_sets;

    for (int i = 0; i < number_of_images; i++) {

        write_descriptor_sets.push_back(VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                             .pNext = nullptr,
                                                             .dstSet = m_descriptor_sets[static_cast<size_t>(i)],
                                                             .dstBinding = 0,
                                                             .dstArrayElement = 0,
                                                             .descriptorCount = 1,
                                                             .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                                             .pImageInfo = nullptr,
                                                             .pBufferInfo = &descriptor_buffer_info_vertex,
                                                             .pTexelBufferView = nullptr});

        if (uniform_buffers.size() > 0) {
            VkDescriptorBufferInfo descriptor_buffer_info_uniform = {
                .buffer = uniform_buffers[static_cast<size_t>(i)].p_buffer,
                .offset = 0,
                .range = static_cast<VkDeviceSize>(uniform_data_size),
            };

            write_descriptor_sets.push_back(VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                                 .pNext = nullptr,
                                                                 .dstSet = m_descriptor_sets[static_cast<size_t>(i)],
                                                                 .dstBinding = 1,
                                                                 .dstArrayElement = 0,
                                                                 .descriptorCount = 1,
                                                                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                 .pImageInfo = nullptr,
                                                                 .pBufferInfo = &descriptor_buffer_info_uniform,
                                                                 .pTexelBufferView = nullptr});
        }

        if (mesh_ptr->p_texture) {
            write_descriptor_sets.push_back(
                VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                     .dstSet = m_descriptor_sets[i],
                                     .dstBinding = 2,
                                     .dstArrayElement = 0,
                                     .descriptorCount = 1,
                                     .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                     .pImageInfo = &image_info});
        }
    }

    vkUpdateDescriptorSets(p_device, static_cast<u32>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0,
                           nullptr);
}

} // namespace vk
} // namespace gouda