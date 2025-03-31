#include "renderers/vulkan/vk_graphics_pipeline.hpp"

#include "core/types.hpp"
#include "debug/assert.hpp"
#include "debug/logger.hpp"
#include "renderers/vulkan/vk_mesh.hpp"
#include "renderers/vulkan/vk_renderer.hpp"
#include "renderers/vulkan/vk_utils.hpp"

namespace gouda::vk {

GraphicsPipeline::GraphicsPipeline(VulkanRenderer &renderer, GLFWwindow *p_window, VkRenderPass p_render_pass,
                                   VkShaderModule p_vertex_shader, VkShaderModule p_fragment_shader,
                                   const Mesh *mesh_ptr, int number_of_images, std::vector<Buffer> &uniform_buffers,
                                   int uniform_data_size)
    : m_renderer{renderer},
      p_device{renderer.GetDevice()},
      p_pipeline{VK_NULL_HANDLE},
      p_pipeline_layout{VK_NULL_HANDLE},
      p_descriptor_pool{VK_NULL_HANDLE},
      p_descriptor_set_layout{VK_NULL_HANDLE}
{
    ASSERT(mesh_ptr, "Mesh pointer cannot be null for pipeline creation");

    // Create descriptor sets with the renderer's static vertex buffer and instance buffers
    CreateDescriptorSets(mesh_ptr, number_of_images, uniform_buffers, uniform_data_size);

    // Shader stages
    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stage_create_info = {
        VkPipelineShaderStageCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                        .stage = VK_SHADER_STAGE_VERTEX_BIT,
                                        .module = p_vertex_shader,
                                        .pName = "main"},
        VkPipelineShaderStageCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                                        .module = p_fragment_shader,
                                        .pName = "main"}};

    // Vertex input state for static vertices and instance data
    std::vector<VkVertexInputBindingDescription> binding_descriptions = {
        // Binding 0: Static vertex buffer (per-vertex data)
        VkVertexInputBindingDescription{
            .binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
        // Binding 1: Instance buffer (per-instance data)
        VkVertexInputBindingDescription{
            .binding = 1, .stride = sizeof(InstanceData), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE}};

    std::vector<VkVertexInputAttributeDescription> attribute_descriptions = {
        // Binding 0: Vertex position (vec3)
        VkVertexInputAttributeDescription{
            .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, position)},
        // Binding 0: Vertex UV (vec2)
        VkVertexInputAttributeDescription{
            .location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv)},
        // Binding 0: Vertex color (vec4)
        VkVertexInputAttributeDescription{
            .location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Vertex, colour)},
        // Binding 1: Instance position (vec3)
        VkVertexInputAttributeDescription{.location = 3,
                                          .binding = 1,
                                          .format = VK_FORMAT_R32G32B32_SFLOAT,
                                          .offset = offsetof(InstanceData, position)},
        // Binding 1: Instance scale (float)
        VkVertexInputAttributeDescription{
            .location = 4, .binding = 1, .format = VK_FORMAT_R32_SFLOAT, .offset = offsetof(InstanceData, scale)},
        // Binding 1: Instance rotation (float)
        VkVertexInputAttributeDescription{
            .location = 5, .binding = 1, .format = VK_FORMAT_R32_SFLOAT, .offset = offsetof(InstanceData, rotation)}};

    VkPipelineVertexInputStateCreateInfo vertex_input_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<u32>(binding_descriptions.size()),
        .pVertexBindingDescriptions = binding_descriptions.data(),
        .vertexAttributeDescriptionCount = static_cast<u32>(attribute_descriptions.size()),
        .pVertexAttributeDescriptions = attribute_descriptions.data()};

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        //.topology = PrimitiveTopologyToVulkan(mesh_ptr->m_topology),
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE};

    VkDynamicState dynamic_states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state_info{.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                                        .dynamicStateCount = 2,
                                                        .pDynamicStates = dynamic_states};

    VkPipelineViewportStateCreateInfo viewport_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .scissorCount = 1};

    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f};

    VkPipelineMultisampleStateCreateInfo pipeline_multisampler_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE};

    VkPipelineColorBlendAttachmentState blend_attach_state{.blendEnable = VK_FALSE,
                                                           .colorWriteMask =
                                                               VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

    VkPipelineColorBlendStateCreateInfo blend_create_info{.sType =
                                                              VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                                          .attachmentCount = 1,
                                                          .pAttachments = &blend_attach_state};

    VkPipelineLayoutCreateInfo layout_info{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                           .setLayoutCount = 1,
                                           .pSetLayouts = &p_descriptor_set_layout};

    VkResult result = vkCreatePipelineLayout(p_device, &layout_info, nullptr, &p_pipeline_layout);
    CHECK_VK_RESULT(result, "vkCreatePipelineLayout\n");

    VkGraphicsPipelineCreateInfo pipeline_info{.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                                               .stageCount = static_cast<u32>(shader_stage_create_info.size()),
                                               .pStages = shader_stage_create_info.data(),
                                               .pVertexInputState = &vertex_input_info,
                                               .pInputAssemblyState = &pipeline_input_assembly_state_create_info,
                                               .pViewportState = &viewport_state_create_info,
                                               .pRasterizationState = &pipeline_rasterization_state_create_info,
                                               .pMultisampleState = &pipeline_multisampler_create_info,
                                               .pDepthStencilState = &depth_stencil_state_create_info,
                                               .pColorBlendState = &blend_create_info,
                                               .pDynamicState = &dynamic_state_info,
                                               .layout = p_pipeline_layout,
                                               .renderPass = p_render_pass,
                                               .subpass = 0};

    result = vkCreateGraphicsPipelines(p_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &p_pipeline);
    CHECK_VK_RESULT(result, "vkCreateGraphicsPipelines\n");

    ENGINE_LOG_DEBUG("Graphics pipeline created");
}

GraphicsPipeline::~GraphicsPipeline() { Destroy(); }

void GraphicsPipeline::Bind(VkCommandBuffer command_buffer_ptr, size_t image_index) const
{
    vkCmdBindPipeline(command_buffer_ptr, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline);
    if (!m_descriptor_sets.empty()) {
        vkCmdBindDescriptorSets(command_buffer_ptr, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline_layout, 0, 1,
                                &m_descriptor_sets[image_index], 0, nullptr);
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
    // Account for all possible descriptors for each image
    std::vector<VkDescriptorPoolSize> pool_sizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 * static_cast<u32>(number_of_images)},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * static_cast<u32>(number_of_images)},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<u32>(number_of_images)}};

    // Multiply number_of_images by the number of bindings per set
    VkDescriptorPoolCreateInfo descriptor_pool_create_info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                           .maxSets =
                                                               static_cast<u32>(number_of_images), // One set per image
                                                           .poolSizeCount = static_cast<u32>(pool_sizes.size()),
                                                           .pPoolSizes = pool_sizes.data()};

    VkResult result = vkCreateDescriptorPool(p_device, &descriptor_pool_create_info, nullptr, &p_descriptor_pool);
    CHECK_VK_RESULT(result, "vkCreateDescriptorPool");
    ENGINE_LOG_DEBUG("Descriptor pool created successfully");
}

void GraphicsPipeline::CreateDescriptorSets(const Mesh *mesh_ptr, int number_of_images,
                                            std::vector<Buffer> &uniform_buffers, int uniform_data_size)
{
    CreateDescriptorPool(number_of_images);
    CreateDescriptorSetLayout(uniform_buffers, uniform_data_size, mesh_ptr->p_texture.get());
    AllocateDescriptorSets(number_of_images);
    UpdateDescriptorSets(mesh_ptr, number_of_images, uniform_buffers, uniform_data_size);
}

void GraphicsPipeline::CreateDescriptorSetLayout(std::vector<Buffer> &uniform_buffers, int uniform_data_size,
                                                 VulkanTexture *texture_ptr)
{
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings = {
        // Binding 0: Static vertex buffer (storage buffer)
        VkDescriptorSetLayoutBinding{.binding = 0,
                                     .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                     .descriptorCount = 1,
                                     .stageFlags = VK_SHADER_STAGE_VERTEX_BIT},
        // Binding 1: Instance buffer (storage buffer)
        VkDescriptorSetLayoutBinding{.binding = 1,
                                     .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                     .descriptorCount = 1,
                                     .stageFlags = VK_SHADER_STAGE_VERTEX_BIT},
        // Binding 2: Uniform buffer
        VkDescriptorSetLayoutBinding{.binding = 2,
                                     .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                     .descriptorCount = 1,
                                     .stageFlags = VK_SHADER_STAGE_VERTEX_BIT}};

    if (texture_ptr) {
        layout_bindings.push_back(
            VkDescriptorSetLayoutBinding{.binding = 3,
                                         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                         .descriptorCount = 1,
                                         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT});
    }

    VkDescriptorSetLayoutCreateInfo layout_info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                .bindingCount = static_cast<u32>(layout_bindings.size()),
                                                .pBindings = layout_bindings.data()};

    VkResult result = vkCreateDescriptorSetLayout(p_device, &layout_info, nullptr, &p_descriptor_set_layout);
    CHECK_VK_RESULT(result, "vkCreateDescriptorSetLayout");
}

void GraphicsPipeline::AllocateDescriptorSets(int number_of_images)
{
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts(number_of_images, p_descriptor_set_layout);
    VkDescriptorSetAllocateInfo descriptor_set_allocation_info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                               .descriptorPool = p_descriptor_pool,
                                                               .descriptorSetCount = static_cast<u32>(number_of_images),
                                                               .pSetLayouts = descriptor_set_layouts.data()};

    m_descriptor_sets.resize(number_of_images);
    VkResult result = vkAllocateDescriptorSets(p_device, &descriptor_set_allocation_info, m_descriptor_sets.data());
    CHECK_VK_RESULT(result, "vkAllocateDescriptorSets");
}

void GraphicsPipeline::UpdateDescriptorSets(const Mesh *mesh_ptr, int number_of_images,
                                            std::vector<Buffer> &uniform_buffers, int uniform_data_size)
{
    ASSERT(mesh_ptr, "Mesh pointer cannot be null");
    ASSERT(uniform_buffers.size() >= static_cast<size_t>(number_of_images), "Not enough uniform buffers");

    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    VkDescriptorImageInfo image_info{};
    if (mesh_ptr->p_texture) {
        image_info.sampler = mesh_ptr->p_texture->p_sampler;
        image_info.imageView = mesh_ptr->p_texture->p_view;
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    for (int i = 0; i < number_of_images; ++i) {
        // Binding 0: Uniform buffer
        VkDescriptorBufferInfo uniform_buffer_info{
            .buffer = uniform_buffers[i].p_buffer, .offset = 0, .range = static_cast<VkDeviceSize>(uniform_data_size)};
        write_descriptor_sets.push_back(VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                             .dstSet = m_descriptor_sets[i],
                                                             .dstBinding = 0,
                                                             .dstArrayElement = 0,
                                                             .descriptorCount = 1,
                                                             .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                             .pBufferInfo = &uniform_buffer_info});

        // Binding 3: Texture (if present)
        if (mesh_ptr->p_texture) {
            write_descriptor_sets.push_back(
                VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                     .dstSet = m_descriptor_sets[i],
                                     .dstBinding = 3,
                                     .dstArrayElement = 0,
                                     .descriptorCount = 1,
                                     .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                     .pImageInfo = &image_info});
        }
    }

    vkUpdateDescriptorSets(p_device, static_cast<u32>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0,
                           nullptr);
}

} // namespace gouda::vk