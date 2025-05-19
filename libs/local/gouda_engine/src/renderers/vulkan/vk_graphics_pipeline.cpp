/**
 * @file vk_graphics_pipeline.cpp
 * @author GoudaCheeseburgers
 * @date 2025-05-12
 * @brief Engine Vulkan graphics pipeline implementation.
 */
#include "renderers/vulkan/vk_graphics_pipeline.hpp"

#include <unordered_set>

#include <GLFW/glfw3.h>

#include "containers/small_vector.hpp"
#include "core/types.hpp"
#include "debug/debug.hpp"
#include "math/math.hpp"
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
u32 get_struct_offset(StringView name, const VkFormat format, bool &success)
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

static constexpr std::string_view pipeline_type_to_string(const PipelineType type)
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
GraphicsPipeline::GraphicsPipeline(Renderer &renderer, VkRenderPass p_render_pass,
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

    auto shader_stages = SetupShaderStages();
    auto vertex_input = SetupVertexInput();
    auto pipeline_states = SetupPipelineStates();
    auto push_constants = SetupPushConstants();

    VkPipelineLayoutCreateInfo layout_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<u32>(m_descriptor_set_layouts.size()),
        .pSetLayouts = m_descriptor_set_layouts.empty() ? nullptr : m_descriptor_set_layouts.data(),
        .pushConstantRangeCount = static_cast<u32>(push_constants.size()),
        .pPushConstantRanges = push_constants.empty() ? nullptr : push_constants.data()};

    VkResult result{vkCreatePipelineLayout(p_device, &layout_info, nullptr, &p_pipeline_layout)};
    if (result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to create graphics pipeline layout. Error code: {}", vk_result_to_string(result));
        CHECK_VK_RESULT(result, "vkCreatePipelineLayout");
    }

    VkGraphicsPipelineCreateInfo pipeline_info{.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                                               .stageCount = static_cast<u32>(shader_stages.size()),
                                               .pStages = shader_stages.data(),
                                               .pVertexInputState = &vertex_input,
                                               .pInputAssemblyState = &pipeline_states.input_assembly,
                                               .pViewportState = &pipeline_states.viewport,
                                               .pRasterizationState = &pipeline_states.rasterization,
                                               .pMultisampleState = &pipeline_states.multisample,
                                               .pDepthStencilState = &pipeline_states.depth_stencil,
                                               .pColorBlendState = &pipeline_states.color_blend,
                                               .pDynamicState = &pipeline_states.dynamic,
                                               .layout = p_pipeline_layout,
                                               .renderPass = p_render_pass,
                                               .subpass = 0};

    result = vkCreateGraphicsPipelines(p_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &p_pipeline);
    if (result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to create graphics pipeline. Error code: {}", vk_result_to_string(result));
        CHECK_VK_RESULT(result, "vkCreateGraphicsPipelines");
    }
    else {
        ENGINE_LOG_DEBUG("Graphics pipeline created for type: {}", pipeline_type_to_string(type));
    }
}

GraphicsPipeline::~GraphicsPipeline() { Destroy(); }

void GraphicsPipeline::Bind(VkCommandBuffer command_buffer_ptr, const size_t image_index) const
{
    vkCmdBindPipeline(command_buffer_ptr, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline);
    if (!m_descriptor_sets.empty()) {
        Vector<VkDescriptorSet> sets;
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
    m_binding_descriptions.clear();
    m_attribute_descriptions.clear();
    ENGINE_LOG_DEBUG("Graphics pipeline destroyed");
}

void GraphicsPipeline::UpdateTextureDescriptors(const size_t number_of_images,
                                                const std::vector<std::unique_ptr<Texture>> &textures)
{
    ASSERT(!textures.empty(), "Texture vector must contain at least the default texture");
    const Texture *default_texture{textures.front().get()};
    if (textures.size() > MAX_TEXTURES) {
        ENGINE_LOG_WARNING("Texture count ({}) exceeds MAX_TEXTURES ({}); capping at MAX_TEXTURES", textures.size(),
                           MAX_TEXTURES);
    }

    Vector<VkDescriptorImageInfo> image_infos{MAX_TEXTURES};
    for (size_t i = 0; i < MAX_TEXTURES; ++i) {
        const Texture *texture{(i < textures.size()) ? textures[i].get() : default_texture};
        image_infos[i] = {.sampler = texture->p_sampler,
                          .imageView = texture->p_view,
                          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }

    Vector<VkWriteDescriptorSet> write_descriptor_sets;
    write_descriptor_sets.reserve(m_descriptor_sets.size() * number_of_images);

    for (u32 set = 0; set < m_descriptor_sets.size(); ++set) {
        if (m_descriptor_sets[set].empty()) {
            continue;
        }
        bool binding_exists{false};
        for (const auto &binding : p_fragment_shader->Reflection().descriptor_bindings) {
            if (binding.set == set && binding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER &&
                binding.binding == 3) {
                binding_exists = true;
                ENGINE_LOG_DEBUG("Found texture binding: set={}, binding=3, count={}", set, binding.count);
                break;
            }
        }
        if (!binding_exists) {
            continue;
        }

        for (int i = 0; i < number_of_images; ++i) {
            if (m_type != PipelineType::Quad && m_type != PipelineType::Particle) {
                continue;
            }
            for (const auto &binding : p_fragment_shader->Reflection().descriptor_bindings) {
                if (binding.set != set || binding.type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
                    binding.binding != 3) {
                    continue;
                }
                VkWriteDescriptorSet write_descriptor_set{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                          .dstSet = m_descriptor_sets[set][i],
                                                          .dstBinding = binding.binding,
                                                          .dstArrayElement = 0,
                                                          .descriptorCount = binding.count,
                                                          .descriptorType = binding.type,
                                                          .pImageInfo = image_infos.data()};
                if (binding.count > MAX_TEXTURES) {
                    ENGINE_LOG_WARNING(
                        "Texture descriptor set {}, binding {} requests {} samplers, but MAX_TEXTURES is {}",
                        binding.set, binding.binding, binding.count, MAX_TEXTURES);
                    write_descriptor_set.descriptorCount = MAX_TEXTURES;
                }
                write_descriptor_sets.push_back(write_descriptor_set);
            }
        }
    }

    if (!write_descriptor_sets.empty()) {
        vkUpdateDescriptorSets(p_device, static_cast<u32>(write_descriptor_sets.size()), write_descriptor_sets.data(),
                               0, nullptr);
        ENGINE_LOG_DEBUG("Updated {} texture descriptor writes for pipeline type: {}", write_descriptor_sets.size(),
                         pipeline_type_to_string(m_type));
    }
    else {
        ENGINE_LOG_WARNING(
            "No texture descriptor writes generated for pipeline type: {}. Expected binding=3 in fragment shader.",
            pipeline_type_to_string(m_type));
    }
}

void GraphicsPipeline::UpdateFontTextureDescriptors(const size_t number_of_images,
                                                    const Vector<std::unique_ptr<Texture>> &font_textures)
{
    ASSERT(!font_textures.empty(), "Font texture vector must contain at least the default font texture");
    const Texture *default_texture{(font_textures[0].get())};
    if (font_textures.size() > MAX_TEXTURES) {
        ENGINE_LOG_WARNING("Font texture count ({}) exceeds MAX_TEXTURES ({}); capping at MAX_TEXTURES",
                           font_textures.size(), MAX_TEXTURES);
    }

    Vector<VkDescriptorImageInfo> image_infos{MAX_TEXTURES};
    for (size_t i = 0; i < MAX_TEXTURES; ++i) {
        const Texture *texture{i < font_textures.size() ? font_textures[i].get() : default_texture};
        image_infos[i] = {.sampler = texture->p_sampler,
                          .imageView = texture->p_view,
                          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }

    Vector<VkWriteDescriptorSet> write_descriptor_sets;
    write_descriptor_sets.reserve(m_descriptor_sets.size() * number_of_images);

    for (u32 set = 0; set < m_descriptor_sets.size(); ++set) {
        if (m_descriptor_sets[set].empty()) {
            continue;
        }
        bool binding_exists{false};
        for (const auto &binding : p_fragment_shader->Reflection().descriptor_bindings) {
            if (binding.set == set && binding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER &&
                binding.binding == 4) {
                binding_exists = true;
                ENGINE_LOG_DEBUG("Found font texture binding: set={}, binding=4, count={}", set, binding.count);
                break;
            }
        }
        if (!binding_exists) {
            continue;
        }

        for (int i = 0; i < number_of_images; ++i) {
            if (m_type != PipelineType::Text) {
                continue;
            }
            for (const auto &binding : p_fragment_shader->Reflection().descriptor_bindings) {
                if (binding.set != set || binding.type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
                    binding.binding != 4) {
                    continue;
                }
                VkWriteDescriptorSet write_descriptor_set{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                          .dstSet = m_descriptor_sets[set][i],
                                                          .dstBinding = binding.binding,
                                                          .dstArrayElement = 0,
                                                          .descriptorCount = binding.count,
                                                          .descriptorType = binding.type,
                                                          .pImageInfo = image_infos.data()};
                if (binding.count > MAX_TEXTURES) {
                    ENGINE_LOG_WARNING(
                        "Font descriptor set {}, binding {} requests {} samplers, but MAX_TEXTURES is {}", binding.set,
                        binding.binding, binding.count, MAX_TEXTURES);
                    write_descriptor_set.descriptorCount = MAX_TEXTURES;
                }
                write_descriptor_sets.push_back(write_descriptor_set);
            }
        }
    }

    if (!write_descriptor_sets.empty()) {
        vkUpdateDescriptorSets(p_device, static_cast<u32>(write_descriptor_sets.size()), write_descriptor_sets.data(),
                               0, nullptr);
        ENGINE_LOG_DEBUG("Updated {} font texture descriptor writes for pipeline type: {}",
                         write_descriptor_sets.size(), pipeline_type_to_string(m_type));
    }
    else {
        ENGINE_LOG_WARNING(
            "No font texture descriptor writes generated for pipeline type: {}. Expected binding=4 in fragment shader.",
            pipeline_type_to_string(m_type));
    }
}

// Private functions ---------------------------------------------------------------
void GraphicsPipeline::CreateDescriptorPool(const int number_of_images)
{
    std::unordered_map<VkDescriptorType, u32> pool_size_counts;
    for (const auto &shader : {p_vertex_shader, p_fragment_shader}) {
        for (const auto &binding : shader->Reflection().descriptor_bindings) {
            pool_size_counts[binding.type] += binding.count * number_of_images;
        }
    }

    Vector<VkDescriptorPoolSize> pool_sizes;
    for (const auto &[type, count] : pool_size_counts) {
        pool_sizes.push_back({.type = type, .descriptorCount = count});
    }
    if (pool_sizes.empty()) {
        ENGINE_LOG_WARNING("No descriptor bindings found; creating minimal descriptor pool");
        pool_sizes.push_back({.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1});
    }

    u32 max_set{0};
    for (const auto &shader : {p_vertex_shader, p_fragment_shader}) {
        for (const auto &binding : shader->Reflection().descriptor_bindings) {
            max_set = math::max(max_set, binding.set);
        }
    }
    u32 total_sets{(max_set + 1) * number_of_images};

    const VkDescriptorPoolCreateInfo pool_info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                         .maxSets = total_sets,
                                         .poolSizeCount = static_cast<u32>(pool_sizes.size()),
                                         .pPoolSizes = pool_sizes.data()};

    if (const VkResult result{vkCreateDescriptorPool(p_device, &pool_info, nullptr, &p_descriptor_pool)};
        result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to create descriptor pool. Error: {}", vk_result_to_string(result));
        CHECK_VK_RESULT(result, "vkCreateDescriptorPool");
    }

    ENGINE_LOG_DEBUG("Created descriptor pool with {} sets and {} pool sizes", total_sets, pool_sizes.size());
}

void GraphicsPipeline::CreateDescriptorSets(const int number_of_images, std::vector<Buffer> &uniform_buffers,
                                            const int uniform_data_size)
{
    CreateDescriptorPool(number_of_images);
    CreateDescriptorSetLayout();
    AllocateDescriptorSets(number_of_images);
    UpdateDescriptorSets(number_of_images, uniform_buffers, uniform_data_size);
    UpdateTextureDescriptors(number_of_images, m_renderer.GetTextures());
    UpdateFontTextureDescriptors(number_of_images, m_renderer.GetFontTextures());
}

void GraphicsPipeline::CreateDescriptorSetLayout()
{
    u32 max_set{0};
    for (const auto &shader : {p_vertex_shader, p_fragment_shader}) {
        for (const auto &binding : shader->Reflection().descriptor_bindings) {
            max_set = math::max(max_set, binding.set);
        }
    }
    m_descriptor_set_layouts.resize(max_set + 1, VK_NULL_HANDLE);

    for (u32 set = 0; set <= max_set; ++set) {
        Vector<VkDescriptorSetLayoutBinding> layout_bindings;
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
                VkDescriptorSetLayoutBinding layout_binding{.binding = binding.binding,
                                                            .descriptorType = binding.type,
                                                            .descriptorCount = binding.count,
                                                            .stageFlags = binding.stage_flags,
                                                            .pImmutableSamplers = nullptr};
                layout_bindings.push_back(layout_binding);
            }
        }

        VkDescriptorSetLayoutCreateInfo layout_info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                    .bindingCount = static_cast<u32>(layout_bindings.size()),
                                                    .pBindings =
                                                        layout_bindings.empty() ? nullptr : layout_bindings.data()};
        if (const VkResult result{
                vkCreateDescriptorSetLayout(p_device, &layout_info, nullptr, &m_descriptor_set_layouts[set])};
            result != VK_SUCCESS) {
            ENGINE_LOG_ERROR("Failed to create descriptor set layout for set {}. Error: {}", set,
                             vk_result_to_string(result));
            CHECK_VK_RESULT(result, "vkCreateDescriptorSetLayout");
        }
        ENGINE_LOG_DEBUG("Created descriptor set layout for set {} with {} bindings", set, layout_bindings.size());
    }
}

void GraphicsPipeline::AllocateDescriptorSets(int number_of_images)
{
    m_descriptor_sets.resize(m_descriptor_set_layouts.size());
    for (size_t set = 0; set < m_descriptor_set_layouts.size(); ++set) {
        if (m_descriptor_set_layouts[set] == VK_NULL_HANDLE) {
            continue;
        }
        Vector<VkDescriptorSetLayout> layouts(number_of_images, m_descriptor_set_layouts[set]);
        VkDescriptorSetAllocateInfo alloc_info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                               .descriptorPool = p_descriptor_pool,
                                               .descriptorSetCount = static_cast<u32>(number_of_images),
                                               .pSetLayouts = layouts.data()};
        m_descriptor_sets[set].resize(number_of_images);
        if (const VkResult result{vkAllocateDescriptorSets(p_device, &alloc_info, m_descriptor_sets[set].data())};
            result != VK_SUCCESS) {
            ENGINE_LOG_ERROR("Failed to allocate descriptor set {}. Error: {}", set, vk_result_to_string(result));
            CHECK_VK_RESULT(result, "vkAllocateDescriptorSets");
        }
    }
}

void GraphicsPipeline::UpdateDescriptorSets(int number_of_images, std::vector<Buffer> &uniform_buffers,
                                            int uniform_data_size)
{
    ASSERT(uniform_buffers.size() >= static_cast<size_t>(number_of_images), "Not enough uniform buffers");

    SmallVector<VkWriteDescriptorSet, 10, GrowthPolicyDouble> write_descriptor_sets;
    SmallVector<std::unique_ptr<VkDescriptorBufferInfo>, 10> buffer_infos;

    for (u32 set = 0; set < m_descriptor_sets.size(); ++set) {
        if (m_descriptor_sets[set].empty()) {
            continue;
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
                        continue;
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

    if (!write_descriptor_sets.empty()) {
        vkUpdateDescriptorSets(p_device, static_cast<u32>(write_descriptor_sets.size()), write_descriptor_sets.data(),
                               0, nullptr);
        ENGINE_LOG_DEBUG("Updated {} uniform buffer descriptor writes", write_descriptor_sets.size());
    }
    else {
        ENGINE_LOG_WARNING("No uniform buffer descriptor writes generated; expected set=0, binding=0");
    }
}

std::array<VkPipelineShaderStageCreateInfo, 2> GraphicsPipeline::SetupShaderStages() const
{
    SmallVector<VkSpecializationMapEntry, 4> spec_entries_vertex;
    SmallVector<u8, 4> spec_data_vertex;
    for (const auto &spec : p_vertex_shader->Reflection().specialization_constants) {
        VkSpecializationMapEntry entry{.constantID = spec.constant_id,
                                       .offset = static_cast<u32>(spec_data_vertex.size()),
                                       .size = spec.default_value.size()};
        spec_entries_vertex.push_back(entry);
        spec_data_vertex.insert(spec_data_vertex.end(), spec.default_value.begin(), spec.default_value.end());
    }

    SmallVector<VkSpecializationMapEntry, 4> spec_entries_fragment;
    SmallVector<u8, 4> spec_data_fragment;
    for (const auto &spec : p_fragment_shader->Reflection().specialization_constants) {
        VkSpecializationMapEntry entry{.constantID = spec.constant_id,
                                       .offset = static_cast<u32>(spec_data_fragment.size()),
                                       .size = spec.default_value.size()};
        spec_entries_fragment.push_back(entry);
        spec_data_fragment.insert(spec_data_fragment.end(), spec.default_value.begin(), spec.default_value.end());
    }

    const VkSpecializationInfo spec_info_vertex{.mapEntryCount = static_cast<u32>(spec_entries_vertex.size()),
                                                .pMapEntries =
                                                    spec_entries_vertex.empty() ? nullptr : spec_entries_vertex.data(),
                                                .dataSize = spec_data_vertex.size(),
                                                .pData = spec_data_vertex.empty() ? nullptr : spec_data_vertex.data()};

    const VkSpecializationInfo spec_info_fragment{.mapEntryCount = static_cast<u32>(spec_entries_fragment.size()),
                                            .pMapEntries =
                                                spec_entries_fragment.empty() ? nullptr : spec_entries_fragment.data(),
                                            .dataSize = spec_data_fragment.size(),
                                            .pData = spec_data_fragment.empty() ? nullptr : spec_data_fragment.data()};

    return {VkPipelineShaderStageCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                            .stage = p_vertex_shader->Stage(),
                                            .module = p_vertex_shader->Get(),
                                            .pName = p_vertex_shader->Reflection().entry_point.c_str(),
                                            .pSpecializationInfo =
                                                spec_entries_vertex.empty() ? nullptr : &spec_info_vertex},
            VkPipelineShaderStageCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                            .stage = p_fragment_shader->Stage(),
                                            .module = p_fragment_shader->Get(),
                                            .pName = p_fragment_shader->Reflection().entry_point.c_str(),
                                            .pSpecializationInfo =
                                                spec_entries_fragment.empty() ? nullptr : &spec_info_fragment}};
}

VkPipelineVertexInputStateCreateInfo GraphicsPipeline::SetupVertexInput()
{
    m_binding_descriptions.clear();
    m_attribute_descriptions.clear();
    u32 attribute_index{0};

    // Binding 0: Per-vertex data (Vertex struct)
    m_binding_descriptions.push_back(
        {.binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX});
    ENGINE_LOG_DEBUG("Added vertex binding: binding=0, stride={}, inputRate=Vertex", sizeof(Vertex));

    for (const auto &[location, name, format, input_rate] : p_vertex_shader->Reflection().vertex_inputs) {
        if (input_rate != VK_VERTEX_INPUT_RATE_VERTEX) {
            continue;
        }
        bool success{false};
        u32 offset = internal::get_struct_offset<Vertex>(name, format, success);
        if (!success) {
            ENGINE_LOG_WARNING("Unsupported vertex input: name={}, format={}", name,
                               vk_format_to_string_view(format));
            continue;
        }
        m_attribute_descriptions.push_back(
            {.location = location, .binding = 0, .format = format, .offset = offset});
        ENGINE_LOG_DEBUG("Added vertex attribute: name={}, location={}, format={}, offset={}", name,
                         location, vk_format_to_string_view(format), offset);
        attribute_index++;
    }

    // Binding 1: Per-instance data
    if (m_type == PipelineType::Quad) {
        m_binding_descriptions.push_back(
            {.binding = 1, .stride = sizeof(InstanceData), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE});
        ENGINE_LOG_DEBUG("Added instance binding (Quad): binding=1, stride={}, inputRate=Instance",
                         sizeof(InstanceData));
        for (const auto &[location, name, format, input_rate] : p_vertex_shader->Reflection().vertex_inputs) {
            if (input_rate != VK_VERTEX_INPUT_RATE_INSTANCE) {
                continue;
            }
            bool success{false};
            u32 offset = internal::get_struct_offset<InstanceData>(name, format, success);
            if (!success) {
                ENGINE_LOG_WARNING("Unsupported instance input (Quad): name={}, format={}", name,
                                   vk_format_to_string_view(format));
                continue;
            }
            m_attribute_descriptions.push_back(
                {.location = location, .binding = 1, .format = format, .offset = offset});
            ENGINE_LOG_DEBUG("Added instance attribute (Quad): name={}, location={}, format={}, offset={}", name,
                             location, vk_format_to_string_view(format), offset);
            attribute_index++;
        }
    }
    else if (m_type == PipelineType::Text) {
        m_binding_descriptions.push_back(
            {.binding = 1, .stride = sizeof(TextData), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE});
        ENGINE_LOG_DEBUG("Added instance binding (Text): binding=1, stride={}, inputRate=Instance", sizeof(TextData));
        for (const auto &[location, name, format, input_rate] : p_vertex_shader->Reflection().vertex_inputs) {
            if (input_rate != VK_VERTEX_INPUT_RATE_INSTANCE) {
                continue;
            }
            bool success{false};
            u32 offset = internal::get_struct_offset<TextData>(name, format, success);
            if (!success) {
                ENGINE_LOG_WARNING("Unsupported instance input (Text): name={}, format={}", name,
                                   vk_format_to_string_view(format));
                continue;
            }
            m_attribute_descriptions.push_back(
                {.location = location, .binding = 1, .format = format, .offset = offset});
            ENGINE_LOG_DEBUG("Added instance attribute (Text): name={}, location={}, format={}, offset={}", name,
                             location, vk_format_to_string_view(format), offset);
            attribute_index++;
        }
    }
    else if (m_type == PipelineType::Particle) {
        m_binding_descriptions.push_back(
            {.binding = 1, .stride = sizeof(ParticleData), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE});
        ENGINE_LOG_DEBUG("Added instance binding (Particle): binding=1, stride={}, inputRate=Instance",
                         sizeof(ParticleData));
        for (const auto &[location, name, format, input_rate] : p_vertex_shader->Reflection().vertex_inputs) {
            if (input_rate != VK_VERTEX_INPUT_RATE_INSTANCE) {
                continue;
            }
            bool success{false};
            u32 offset = internal::get_struct_offset<ParticleData>(name, format, success);
            if (!success) {
                ENGINE_LOG_WARNING("Unsupported instance input (Particle): name={}, format={}", name,
                                   vk_format_to_string_view(format));
                continue;
            }
            m_attribute_descriptions.push_back(
                {.location = location, .binding = 1, .format = format, .offset = offset});
            ENGINE_LOG_DEBUG("Added instance attribute (Particle): name={}, location={}, format={}, offset={}",
                             name, location, vk_format_to_string_view(format), offset);
            attribute_index++;
        }
    }

    // Validate bindings and attributes
    std::unordered_set<u32> seen_bindings;
    for (const auto &[binding, stride, inputRate] : m_binding_descriptions) {
        if (!seen_bindings.insert(binding).second) {
            ENGINE_LOG_ERROR("Duplicate binding found: binding={}", binding);
            ENGINE_THROW("Duplicate binding");
        }
        ENGINE_LOG_DEBUG("Final binding: binding={}, stride={}, inputRate={}", binding, stride,
                         inputRate == VK_VERTEX_INPUT_RATE_VERTEX ? "Vertex" : "Instance");
    }

    std::unordered_set<u32> seen_locations;
    for (const auto &attr : m_attribute_descriptions) {
        if (attr.format == VK_FORMAT_UNDEFINED) {
            ENGINE_LOG_ERROR("Invalid attribute format: location={}, binding={}", attr.location, attr.binding);
            ENGINE_THROW("Invalid attribute format");
        }
        if (!seen_locations.insert(attr.location).second) {
            ENGINE_LOG_ERROR("Duplicate attribute location: location={}, binding={}", attr.location, attr.binding);
            ENGINE_THROW("Duplicate attribute location");
        }
        ENGINE_LOG_DEBUG("Final attribute: location={}, binding={}, format={}", attr.location, attr.binding,
                         vk_format_to_string_view(attr.format));
    }

    if (m_binding_descriptions.empty()) {
        ENGINE_LOG_ERROR("No valid vertex bindings defined for pipeline type: {}", pipeline_type_to_string(m_type));
        ENGINE_THROW("Failed to create graphics pipeline: no valid vertex bindings");
    }
    if (m_attribute_descriptions.empty()) {
        ENGINE_LOG_ERROR("No valid vertex or instance attributes found for pipeline type: {}",
                         pipeline_type_to_string(m_type));
        ENGINE_THROW("Failed to create graphics pipeline: no valid vertex attributes");
    }

    ENGINE_LOG_DEBUG("SetupVertexInput: {} bindings, {} attributes for pipeline type: {}",
                     m_binding_descriptions.size(), m_attribute_descriptions.size(), pipeline_type_to_string(m_type));

    const VkPipelineVertexInputStateCreateInfo vertex_input_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<u32>(m_binding_descriptions.size()),
        .pVertexBindingDescriptions = m_binding_descriptions.data(),
        .vertexAttributeDescriptionCount = static_cast<u32>(m_attribute_descriptions.size()),
        .pVertexAttributeDescriptions = m_attribute_descriptions.data()};

    ASSERT(vertex_input_info.pVertexBindingDescriptions != nullptr, "Invalid binding descriptions pointer");
    ASSERT(vertex_input_info.pVertexAttributeDescriptions != nullptr, "Invalid attribute descriptions pointer");

    return vertex_input_info;
}

GraphicsPipeline::PipelineStates GraphicsPipeline::SetupPipelineStates() const
{
    PipelineStates states{};
    states.input_assembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                             .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                             .primitiveRestartEnable = VK_FALSE};

    states.viewport = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                       .viewportCount = 1,
                       .pViewports = nullptr,
                       .scissorCount = 1,
                       .pScissors = nullptr};

    states.rasterization = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                            .polygonMode = VK_POLYGON_MODE_FILL,
                            .cullMode = VK_CULL_MODE_BACK_BIT,
                            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                            .lineWidth = 1.0f};

    states.multisample = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                          .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

    states.depth_stencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                            .depthTestEnable = VK_TRUE,
                            .depthWriteEnable = VK_TRUE,
                            .depthCompareOp = VK_COMPARE_OP_LESS,
                            .depthBoundsTestEnable = VK_FALSE,
                            .stencilTestEnable = VK_FALSE};

    states.blend_attachment = {
        .blendEnable = m_type == PipelineType::Text || m_type == PipelineType::Particle ? VK_TRUE : VK_FALSE,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

    if (states.blend_attachment.blendEnable) {
        states.blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        states.blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        states.blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        states.blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        states.blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        states.blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    states.color_blend = {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                          .attachmentCount = 1,
                          .pAttachments = &states.blend_attachment};

    static constexpr VkDynamicState dynamic_states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    states.dynamic = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                      .dynamicStateCount = 2,
                      .pDynamicStates = dynamic_states};

    ENGINE_LOG_DEBUG("SetupPipelineStates: {} dynamic states for pipeline type: {}", states.dynamic.dynamicStateCount,
                     pipeline_type_to_string(m_type));

    return states;
}

Vector<VkPushConstantRange> GraphicsPipeline::SetupPushConstants()
{
    Vector<VkPushConstantRange> push_constant_ranges;
    std::unordered_map<VkShaderStageFlags, std::unordered_set<u32>> seen_offsets_by_stage;
    for (const auto &shader : {p_vertex_shader, p_fragment_shader}) {
        auto &seen_offsets = seen_offsets_by_stage[shader->Stage()];
        for (const auto &[offset, size, stage_flags] : shader->Reflection().push_constants) {
            if (offset % 4 != 0 || size % 4 != 0) {
                ENGINE_LOG_ERROR("Push constant in shader stage {} has unaligned offset ({}) or size ({})",
                                 vk_shader_stage_as_string_view(shader->Stage()), offset, size);
                continue;
            }
            const u32 range_end = offset + size;
            for (u32 i = offset; i < range_end; ++i) {
                if (!seen_offsets.insert(i).second) {
                    ENGINE_LOG_ERROR("Overlapping push constant offset {} in shader stage {}", i,
                                     vk_shader_stage_as_string_view(shader->Stage()));
                }
            }
            push_constant_ranges.push_back({.stageFlags = stage_flags, .offset = offset, .size = size});
        }
    }

    u32 total_size{0};
    for (const auto &range : push_constant_ranges) {
        total_size = math::max(total_size, range.offset + range.size);
    }

    if (total_size > 128) {
        ENGINE_LOG_ERROR("Total push constant size {} exceeds typical limit of 128 bytes", total_size);
    }
    return push_constant_ranges;
}

void GraphicsPipeline::PrintReflection() const
{
    for (const auto &[location, name, format, input_rate] : p_vertex_shader->Reflection().vertex_inputs) {
        ENGINE_LOG_DEBUG("Vertex input: name={}, location={}, format={}, input_rate={}", name, location,
                         vk_format_to_string_view(format),
                         input_rate == VK_VERTEX_INPUT_RATE_VERTEX ? "Vertex" : "Instance");
    }
}

} // namespace gouda::vk
