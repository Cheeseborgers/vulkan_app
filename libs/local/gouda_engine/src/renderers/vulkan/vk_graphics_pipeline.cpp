/**
 * @file renderers/vulkan/vk_graphics_pipeline.cpp
 * @author GoudaCheeseburgers
 * @date 2025-04-22
 * @brief Engine Vulkan graphics pipeline implementation.
 */
#include "renderers/vulkan/vk_graphics_pipeline.hpp"

#include <unordered_set>

#include <GLFW/glfw3.h>

#include "containers/small_vector.hpp"
#include "core/types.hpp"
#include "debug/debug.hpp"
#include "renderers/vulkan/vk_buffer.hpp"
#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_renderer.hpp"
#include "renderers/vulkan/vk_shader.hpp"
#include "renderers/vulkan/vk_texture.hpp"
#include "renderers/vulkan/vk_utils.hpp"

namespace gouda::vk {

namespace internal {
// Helper function to map vertex input to struct offsets
template <typename T>
u32 get_struct_offset(const std::string &name, VkFormat format, bool &success)
{
    success = true;
    if constexpr (std::is_same_v<T, Vertex>) {
        if (name == "position" && format == VK_FORMAT_R32G32B32_SFLOAT)
            return offsetof(Vertex, position);
        if (name == "uv" && format == VK_FORMAT_R32G32_SFLOAT)
            return offsetof(Vertex, uv);
    }
    else if constexpr (std::is_same_v<T, InstanceData>) {
        if (name == "instance_position" && format == VK_FORMAT_R32G32B32_SFLOAT)
            return offsetof(InstanceData, position);
        if (name == "instance_size" && format == VK_FORMAT_R32G32_SFLOAT)
            return offsetof(InstanceData, size);
        if (name == "instance_rotation" && format == VK_FORMAT_R32_SFLOAT)
            return offsetof(InstanceData, rotation);
        if (name == "instance_texture_index" && format == VK_FORMAT_R32_UINT)
            return offsetof(InstanceData, texture_index);
        if (name == "instance_colour" && format == VK_FORMAT_R32G32B32A32_SFLOAT)
            return offsetof(InstanceData, colour);
        if (name == "instance_sprite_rect" && format == VK_FORMAT_R32G32B32A32_SFLOAT)
            return offsetof(InstanceData, sprite_rect);
        if (name == "instance_is_atlas" && format == VK_FORMAT_R32_UINT)
            return offsetof(InstanceData, is_atlas);
    }
    else if constexpr (std::is_same_v<T, ParticleData>) {
        if (name == "instance_position" && format == VK_FORMAT_R32G32B32_SFLOAT)
            return offsetof(ParticleData, position);
        if (name == "instance_size" && format == VK_FORMAT_R32G32_SFLOAT)
            return offsetof(ParticleData, size);
        if (name == "instance_colour" && format == VK_FORMAT_R32G32B32A32_SFLOAT)
            return offsetof(ParticleData, colour);
        if (name == "instance_texture_index" && format == VK_FORMAT_R32_UINT)
            return offsetof(ParticleData, texture_index);
        if (name == "instance_lifetime" && format == VK_FORMAT_R32_SFLOAT)
            return offsetof(ParticleData, lifetime);
        if (name == "instance_velocity" && format == VK_FORMAT_R32G32B32_SFLOAT)
            return offsetof(ParticleData, velocity);
    }
    else if constexpr (std::is_same_v<T, TextData>) {
        if (name == "instance_position" && format == VK_FORMAT_R32G32B32_SFLOAT)
            return offsetof(TextData, position);
        if (name == "instance_size" && format == VK_FORMAT_R32G32_SFLOAT)
            return offsetof(TextData, size);
        if (name == "instance_colour" && format == VK_FORMAT_R32G32B32A32_SFLOAT)
            return offsetof(TextData, colour);
        if (name == "instance_glyph_index" && format == VK_FORMAT_R32_UINT)
            return offsetof(TextData, glyph_index);
        if (name == "instance_sdf_params" && format == VK_FORMAT_R32G32B32A32_SFLOAT)
            return offsetof(TextData, sdf_params);
        if (name == "instance_texture_index" && format == VK_FORMAT_R32_UINT)
            return offsetof(TextData, texture_index);
    }

    success = false;
    return 0;
}
} // namespace internal

// TODO: Rename this
static constexpr std::string_view to_string(PipelineType type)
{
    switch (type) {
        case PipelineType::Quad:
            return "Quad/Sprite";
        case PipelineType::Text:
            return "Text";
        case PipelineType::Particle:
            return "Particle";
        default:
            return "Unknown graphics pipeline type! How did we get here?";
    }
}

// GraphicsPipeline implementation -----------------------------------------------------------------
GraphicsPipeline::GraphicsPipeline(Renderer &renderer, GLFWwindow *p_window, VkRenderPass p_render_pass,
                                   Shader *vertex_shader, Shader *fragment_shader, int number_of_images,
                                   std::vector<Buffer> &uniform_buffers, int uniform_data_size, PipelineType type)
    : m_renderer{renderer},
      p_device{renderer.GetDevice()},
      p_pipeline{VK_NULL_HANDLE},
      p_pipeline_layout{VK_NULL_HANDLE},
      p_descriptor_pool{VK_NULL_HANDLE},
      m_type{type},
      p_vertex_shader{vertex_shader},
      p_fragment_shader{fragment_shader}
{
    ASSERT(vertex_shader, "Vertex shader is a null pointer");
    ASSERT(fragment_shader, "Fragment shader is a null pointer");

    CreateDescriptorSets(number_of_images, uniform_buffers, uniform_data_size);

    // Specialization Constants
    SmallVector<VkSpecializationMapEntry, 4> spec_entries_vertex;
    SmallVector<u8, 4> spec_data_vertex;
    for (const auto &spec : vertex_shader->Reflection().specialization_constants) {
        VkSpecializationMapEntry entry{.constantID = spec.constant_id,
                                       .offset = static_cast<u32>(spec_data_vertex.size()),
                                       .size = spec.default_value.size()};
        spec_entries_vertex.push_back(entry);
        spec_data_vertex.insert(spec_data_vertex.end(), spec.default_value.begin(), spec.default_value.end());
    }

    SmallVector<VkSpecializationMapEntry, 4> spec_entries_fragment;
    SmallVector<u8, 4> spec_data_fragment;
    for (const auto &spec : fragment_shader->Reflection().specialization_constants) {
        VkSpecializationMapEntry entry{.constantID = spec.constant_id,
                                       .offset = static_cast<u32>(spec_data_fragment.size()),
                                       .size = spec.default_value.size()};
        spec_entries_fragment.push_back(entry);
        spec_data_fragment.insert(spec_data_fragment.end(), spec.default_value.begin(), spec.default_value.end());
    }

    VkSpecializationInfo spec_info_vertex{.mapEntryCount = static_cast<u32>(spec_entries_vertex.size()),
                                          .pMapEntries =
                                              spec_entries_vertex.empty() ? nullptr : spec_entries_vertex.data(),
                                          .dataSize = spec_data_vertex.size(),
                                          .pData = spec_data_vertex.empty() ? nullptr : spec_data_vertex.data()};

    VkSpecializationInfo spec_info_fragment{.mapEntryCount = static_cast<u32>(spec_entries_fragment.size()),
                                            .pMapEntries =
                                                spec_entries_fragment.empty() ? nullptr : spec_entries_fragment.data(),
                                            .dataSize = spec_data_fragment.size(),
                                            .pData = spec_data_fragment.empty() ? nullptr : spec_data_fragment.data()};

    // Shader stages
    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stage_create_info = {
        VkPipelineShaderStageCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                        .stage = vertex_shader->Stage(),
                                        .module = vertex_shader->Get(),
                                        .pName = vertex_shader->Reflection().entry_point.c_str(),
                                        .pSpecializationInfo =
                                            spec_entries_vertex.empty() ? nullptr : &spec_info_vertex},
        VkPipelineShaderStageCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                        .stage = fragment_shader->Stage(),
                                        .module = fragment_shader->Get(),
                                        .pName = fragment_shader->Reflection().entry_point.c_str(),
                                        .pSpecializationInfo =
                                            spec_entries_fragment.empty() ? nullptr : &spec_info_fragment}};

    // Dynamic Vertex Input
    SmallVector<VkVertexInputBindingDescription, 2> binding_descriptions;
    SmallVector<VkVertexInputAttributeDescription, 9> attribute_descriptions;
    u32 attribute_index{0};

    // Binding 0: Per-vertex data (Vertex struct)
    binding_descriptions.push_back({.binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX});

    for (const auto &input : vertex_shader->Reflection().vertex_inputs) {
        if (input.input_rate != VK_VERTEX_INPUT_RATE_VERTEX)
            continue; // Skip instance inputs
        bool success{false};
        u32 offset{internal::get_struct_offset<Vertex>(input.name, input.format, success)};
        if (!success) {
            ENGINE_LOG_WARNING("Unsupported vertex input: name={}, format={}", input.name,
                               vk_format_to_string_view(input.format));
            continue;
        }
        attribute_descriptions.push_back(
            {.location = input.location, .binding = 0, .format = input.format, .offset = offset});
        attribute_index++;
    }

    // Binding 1: Per-instance data (InstanceData or ParticleData)
    if (type == PipelineType::Quad) {
        binding_descriptions.push_back(
            {.binding = 1, .stride = sizeof(InstanceData), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE});
        for (const auto &input : vertex_shader->Reflection().vertex_inputs) {
            if (input.input_rate != VK_VERTEX_INPUT_RATE_INSTANCE)
                continue;
            bool success{false};
            u32 offset{internal::get_struct_offset<InstanceData>(input.name, input.format, success)};
            if (!success) {
                ENGINE_LOG_WARNING("Unsupported instance input: name={}, format={}", input.name,
                                   vk_format_to_string_view(input.format));
                continue;
            }
            attribute_descriptions.push_back(
                {.location = input.location, .binding = 1, .format = input.format, .offset = offset});
            attribute_index++;
        }
    }
    else if (type == PipelineType::Text) {
        binding_descriptions.push_back(
            {.binding = 1, .stride = sizeof(TextData), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE});
        for (const auto &input : vertex_shader->Reflection().vertex_inputs) {
            if (input.input_rate != VK_VERTEX_INPUT_RATE_INSTANCE)
                continue;
            bool success{false};
            u32 offset{internal::get_struct_offset<TextData>(input.name, input.format, success)};
            if (!success) {
                ENGINE_LOG_WARNING("Unsupported instance input: name={}, format={} in Text Pipeline", input.name,
                                   vk_format_to_string_view(input.format));
                continue;
            }
            attribute_descriptions.push_back(
                {.location = input.location, .binding = 1, .format = input.format, .offset = offset});
            attribute_index++;
        }
    }
    else if (type == PipelineType::Particle) {
        binding_descriptions.push_back(
            {.binding = 1, .stride = sizeof(ParticleData), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE});
        for (const auto &input : vertex_shader->Reflection().vertex_inputs) {
            if (input.input_rate != VK_VERTEX_INPUT_RATE_INSTANCE)
                continue;
            bool success{false};
            u32 offset{internal::get_struct_offset<ParticleData>(input.name, input.format, success)};
            if (!success) {
                ENGINE_LOG_WARNING("Unsupported instance input: name={}, format={}", input.name,
                                   vk_format_to_string_view(input.format));
                continue;
            }
            attribute_descriptions.push_back(
                {.location = input.location, .binding = 1, .format = input.format, .offset = offset});
            attribute_index++;
        }
    }

    if (attribute_descriptions.empty()) {
        ENGINE_LOG_ERROR("No valid vertex or instance inputs found for shader");
        ENGINE_THROW("Failed to create graphics pipeline: no valid vertex inputs");
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<u32>(binding_descriptions.size()),
        .pVertexBindingDescriptions = binding_descriptions.data(),
        .vertexAttributeDescriptionCount = static_cast<u32>(attribute_descriptions.size()),
        .pVertexAttributeDescriptions = attribute_descriptions.data()};

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
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

    VkPipelineColorBlendAttachmentState blend_attach_state{
        .blendEnable = (type == PipelineType::Text || type == PipelineType::Particle) ? VK_TRUE : VK_FALSE,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
    if (blend_attach_state.blendEnable) {
        blend_attach_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend_attach_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_attach_state.colorBlendOp = VK_BLEND_OP_ADD;
        blend_attach_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend_attach_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend_attach_state.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo blend_create_info{.sType =
                                                              VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                                          .attachmentCount = 1,
                                                          .pAttachments = &blend_attach_state};

    // Push Constants
    SmallVector<VkPushConstantRange, 1> push_constant_ranges;
    std::unordered_map<VkShaderStageFlags, std::unordered_set<u32>> seen_offsets_by_stage;
    for (const auto &shader : {vertex_shader, fragment_shader}) {
        auto &seen_offsets = seen_offsets_by_stage[shader->Stage()];
        for (const auto &pc : shader->Reflection().push_constants) {
            if (pc.offset % 4 != 0 || pc.size % 4 != 0) {
                ENGINE_LOG_ERROR("Push constant in shader stage {} has unaligned offset ({}) or size ({})",
                                 vk_shader_stage_as_string_view(shader->Stage()), pc.offset, pc.size);
                continue;
            }
            u32 range_end{pc.offset + pc.size};
            for (u32 i = pc.offset; i < range_end; ++i) {
                if (!seen_offsets.insert(i).second) {
                    ENGINE_LOG_ERROR("Overlapping push constant offset {} in shader stage {}", i,
                                     vk_shader_stage_as_string_view(shader->Stage()));
                }
            }
            push_constant_ranges.push_back({.stageFlags = pc.stage_flags, .offset = pc.offset, .size = pc.size});
        }
    }

    u32 total_size{0};
    for (const auto &range : push_constant_ranges) {
        total_size = std::max(total_size, range.offset + range.size);
    }
    if (total_size > 128) {
        ENGINE_LOG_ERROR("Total push constant size {} exceeds typical limit of 128 bytes", total_size);
    }

    VkPipelineLayoutCreateInfo layout_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<u32>(m_descriptor_set_layouts.size()),
        .pSetLayouts = m_descriptor_set_layouts.empty() ? nullptr : m_descriptor_set_layouts.data(),
        .pushConstantRangeCount = static_cast<u32>(push_constant_ranges.size()),
        .pPushConstantRanges = push_constant_ranges.empty() ? nullptr : push_constant_ranges.data()};

    VkResult result{vkCreatePipelineLayout(p_device, &layout_info, nullptr, &p_pipeline_layout)};
    if (result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to create graphics pipeline layout. Error code: {}", vk_result_to_string(result));
        CHECK_VK_RESULT(result, "vkCreatePipelineLayout");
    }

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
    if (result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to create graphics pipeline. Error code: {}", vk_result_to_string(result));
        CHECK_VK_RESULT(result, "vkCreateGraphicsPipelines");
    }
    else {
        ENGINE_LOG_DEBUG("Graphics pipeline created for type: {}", to_string(type));
    }
}

GraphicsPipeline::~GraphicsPipeline() { Destroy(); }

void GraphicsPipeline::Bind(VkCommandBuffer command_buffer_ptr, size_t image_index) const
{
    vkCmdBindPipeline(command_buffer_ptr, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline);

    if (!m_descriptor_sets.empty()) {
        std::vector<VkDescriptorSet> sets;
        for (const auto &set : m_descriptor_sets) {
            if (image_index < set.size()) {
                sets.push_back(set[image_index]);
            }
        }
        if (!sets.empty()) {
            vkCmdBindDescriptorSets(command_buffer_ptr, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline_layout, 0,
                                    static_cast<u32>(sets.size()), sets.data(), 0, nullptr);
        }
    }
}

void GraphicsPipeline::Destroy()
{
    for (auto &layout : m_descriptor_set_layouts) {
        if (layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(p_device, layout, nullptr);
            layout = VK_NULL_HANDLE;
        }
    }
    m_descriptor_set_layouts.clear();

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

    m_descriptor_sets.clear();

    ENGINE_LOG_DEBUG("Graphics pipeline destroyed");
}

void GraphicsPipeline::CreateDescriptorPool(int number_of_images)
{
    // Count descriptors by type across all sets and shaders
    std::unordered_map<VkDescriptorType, u32> pool_size_counts;
    for (const auto &shader : {p_vertex_shader, p_fragment_shader}) {
        for (const auto &binding : shader->Reflection().descriptor_bindings) {
            pool_size_counts[binding.type] += binding.count * number_of_images;
        }
    }

    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (const auto &[type, count] : pool_size_counts) {
        pool_sizes.push_back({.type = type, .descriptorCount = count});
    }

    if (pool_sizes.empty()) {
        ENGINE_LOG_WARNING("No descriptor bindings found in shaders; creating empty descriptor pool");
        pool_sizes.push_back({.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1});
    }

    // Calculate total number of sets (max set number + 1, per frame)
    u32 max_set{0};
    for (const auto &shader : {p_vertex_shader, p_fragment_shader}) {
        for (const auto &binding : shader->Reflection().descriptor_bindings) {
            max_set = std::max(max_set, binding.set);
        }
    }
    u32 total_sets{(max_set + 1) * number_of_images};

    VkDescriptorPoolCreateInfo descriptor_pool_create_info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                           .maxSets = total_sets,
                                                           .poolSizeCount = static_cast<u32>(pool_sizes.size()),
                                                           .pPoolSizes = pool_sizes.data()};

    VkResult result{vkCreateDescriptorPool(p_device, &descriptor_pool_create_info, nullptr, &p_descriptor_pool)};
    if (result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to create descriptor pool. Error code: {}", vk_result_to_string(result));
        CHECK_VK_RESULT(result, "vkCreateDescriptorPool");
    }
}

void GraphicsPipeline::CreateDescriptorSetLayout()
{
    // Find maximum set number
    u32 max_set = 0;
    for (const auto &shader : {p_vertex_shader, p_fragment_shader}) {
        for (const auto &binding : shader->Reflection().descriptor_bindings) {
            max_set = std::max(max_set, binding.set);
        }
    }

    // Create a layout for each set (0 to max_set)
    m_descriptor_set_layouts.resize(max_set + 1, VK_NULL_HANDLE);
    for (u32 set = 0; set <= max_set; ++set) {
        std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
        std::unordered_set<u32> seen_bindings;
        for (const auto &shader : {p_vertex_shader, p_fragment_shader}) {
            for (const auto &binding : shader->Reflection().descriptor_bindings) {
                if (binding.set != set) {
                    continue;
                }
                if (!seen_bindings.insert(binding.binding).second) {
                    ENGINE_LOG_WARNING("Duplicate descriptor binding {} in set {} in shader stage {}", binding.binding,
                                       set, vk_shader_stage_as_string_view(shader->Stage()));
                    continue;
                }
                VkDescriptorSetLayoutBinding layout_binding{
                    .binding = binding.binding,
                    .descriptorType = binding.type,
                    .descriptorCount = binding.count,
                    .stageFlags = binding.stage_flags,
                    .pImmutableSamplers = nullptr // Assume no immutable samplers
                };
                layout_bindings.push_back(layout_binding);
            }
        }

        if (layout_bindings.empty()) {
            // Create an empty layout for unused sets to maintain set numbering
            VkDescriptorSetLayoutCreateInfo layout_info{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 0, .pBindings = nullptr};
            VkResult result{
                vkCreateDescriptorSetLayout(p_device, &layout_info, nullptr, &m_descriptor_set_layouts[set])};
            if (result != VK_SUCCESS) {
                ENGINE_LOG_ERROR("Failed to create descriptor set layout. Error code: {}", vk_result_to_string(result));
                CHECK_VK_RESULT(result, "vkCreateDescriptorSetLayout");
            }
            continue;
        }

        VkDescriptorSetLayoutCreateInfo layout_info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                    .bindingCount = static_cast<u32>(layout_bindings.size()),
                                                    .pBindings = layout_bindings.data()};

        VkResult result{vkCreateDescriptorSetLayout(p_device, &layout_info, nullptr, &m_descriptor_set_layouts[set])};
        if (result != VK_SUCCESS) {
            ENGINE_LOG_ERROR("Failed to create descriptor set layout. Error code: {}", vk_result_to_string(result));
            CHECK_VK_RESULT(result, "vkCreateDescriptorSetLayout");
        }
    }
}

void GraphicsPipeline::AllocateDescriptorSets(int number_of_images)
{
    m_descriptor_sets.resize(m_descriptor_set_layouts.size());
    for (size_t set = 0; set < m_descriptor_set_layouts.size(); ++set) {
        if (m_descriptor_set_layouts[set] == VK_NULL_HANDLE) {
            continue; // Skip unused sets
        }
        std::vector<VkDescriptorSetLayout> layouts(number_of_images, m_descriptor_set_layouts[set]);
        VkDescriptorSetAllocateInfo alloc_info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                               .descriptorPool = p_descriptor_pool,
                                               .descriptorSetCount = static_cast<u32>(number_of_images),
                                               .pSetLayouts = layouts.data()};
        m_descriptor_sets[set].resize(number_of_images);
        VkResult result{vkAllocateDescriptorSets(p_device, &alloc_info, m_descriptor_sets[set].data())};
        if (result != VK_SUCCESS) {
            ENGINE_LOG_ERROR("Failed to allocate descriptor set. Error code: {}", vk_result_to_string(result));
            CHECK_VK_RESULT(result, "vkAllocateDescriptorSets");
        }
    }
}

void GraphicsPipeline::UpdateDescriptorSets(int number_of_images, std::vector<Buffer> &uniform_buffers,
                                            int uniform_data_size)
{
    ASSERT(uniform_buffers.size() >= static_cast<size_t>(number_of_images), "Not enough uniform buffers");

    const auto &textures = m_renderer.GetTextures();
    std::vector<VkDescriptorImageInfo> image_infos(MAX_TEXTURES);
    Texture *fallback_texture{textures[0].get()};
    for (size_t i = 0; i < MAX_TEXTURES; ++i) {
        Texture *texture = (i < textures.size()) ? textures[i].get() : fallback_texture;
        image_infos[i] = {.sampler = texture->p_sampler,
                          .imageView = texture->p_view,
                          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }

    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    std::vector<std::unique_ptr<VkDescriptorBufferInfo>> buffer_infos;

    for (u32 set = 0; set < m_descriptor_sets.size(); ++set) {
        if (m_descriptor_sets[set].empty()) {
            continue; // Skip unused sets
        }
        for (int i = 0; i < number_of_images; ++i) {
            for (const auto &shader : {p_vertex_shader, p_fragment_shader}) {
                for (const auto &binding : shader->Reflection().descriptor_bindings) {
                    if (binding.set != set) {
                        continue;
                    }
                    VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                               .dstSet = m_descriptor_sets[set][i],
                                               .dstBinding = binding.binding,
                                               .dstArrayElement = 0,
                                               .descriptorCount = binding.count,
                                               .descriptorType = binding.type};

                    if (binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                        // Assume uniform buffer is for set=0, binding=0
                        if (binding.set == 0 && binding.binding == 0) {
                            auto buffer_info = std::make_unique<VkDescriptorBufferInfo>(
                                VkDescriptorBufferInfo{.buffer = uniform_buffers[i].p_buffer,
                                                       .offset = 0,
                                                       .range = static_cast<VkDeviceSize>(uniform_data_size)});
                            write.pBufferInfo = buffer_info.get();
                            buffer_infos.push_back(std::move(buffer_info));
                        }
                        else {
                            ENGINE_LOG_WARNING("No uniform buffer mapped for set {}, binding {}", binding.set,
                                               binding.binding);
                            continue;
                        }
                    }
                    else if (binding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                        // Use texture array for sampler bindings
                        if (binding.count > MAX_TEXTURES) {
                            ENGINE_LOG_WARNING(
                                "Descriptor set {}, binding {} requests {} samplers, but MAX_TEXTURES is {}",
                                binding.set, binding.binding, binding.count, MAX_TEXTURES);
                            write.descriptorCount = MAX_TEXTURES;
                        }
                        write.pImageInfo = image_infos.data();
                    }
                    else {
                        ENGINE_LOG_WARNING("Unsupported descriptor type {} at set {}, binding {}",
                                           vk_descriptor_type_to_string_view(binding.type), binding.set,
                                           binding.binding);
                        continue;
                    }

                    write_descriptor_sets.push_back(write);
                }
            }
        }
    }

    if (write_descriptor_sets.empty()) {
        ENGINE_LOG_WARNING("No valid descriptor writes generated for pipeline");
    }

    vkUpdateDescriptorSets(p_device, static_cast<u32>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0,
                           nullptr);
}

void GraphicsPipeline::CreateDescriptorSets(int number_of_images, std::vector<Buffer> &uniform_buffers,
                                            int uniform_data_size)
{
    CreateDescriptorPool(number_of_images);
    CreateDescriptorSetLayout();
    AllocateDescriptorSets(number_of_images);
    UpdateDescriptorSets(number_of_images, uniform_buffers, uniform_data_size);
}

} // namespace gouda::vk