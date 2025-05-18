/**
 * @file vk_compute_pipeline.cpp
 * @author GoudaCheeseburgers
 * @date 2025-04-15
 * @brief Engine module implementation
 */
#include "renderers/vulkan/vk_compute_pipeline.hpp"

#include "debug/logger.hpp"
#include "renderers/vulkan/vk_renderer.hpp"
#include "renderers/vulkan/vk_shader.hpp"
#include "renderers/vulkan/vk_utils.hpp"
#include "containers/small_vector.hpp"

namespace gouda::vk {

ComputePipeline::ComputePipeline(Renderer &renderer, const Shader *compute_shader, const std::vector<Buffer> &storage_buffers,
                                 const std::vector<Buffer> &uniform_buffers,
                                 const VkDeviceSize storage_buffer_size, const VkDeviceSize uniform_buffer_size)
    : m_renderer{renderer}, p_device{renderer.GetDevice()}
{
    // Create descriptor pool
    Vector<VkDescriptorPoolSize> pool_sizes{
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<u32>(storage_buffers.size())},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<u32>(uniform_buffers.size())},
    };
    const VkDescriptorPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = static_cast<u32>(storage_buffers.size()),
        .poolSizeCount = static_cast<u32>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };
    VkResult result{vkCreateDescriptorPool(p_device, &pool_info, nullptr, &p_descriptor_pool)};
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateDescriptorPool");
    }


    // Create descriptor set layout
    Vector<VkDescriptorSetLayoutBinding> bindings{
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };
    const VkDescriptorSetLayoutCreateInfo layout_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };
    result = vkCreateDescriptorSetLayout(p_device, &layout_info, nullptr, &p_descriptor_set_layout);
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateDescriptorSetLayout");
    }

    // Create pipeline layout
    const VkPipelineLayoutCreateInfo pipeline_layout_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &p_descriptor_set_layout,
    };
    result = vkCreatePipelineLayout(p_device, &pipeline_layout_info, nullptr, &p_pipeline_layout);
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreatePipelineLayout");
    }

    // Create compute pipeline
    const VkComputePipelineCreateInfo pipeline_info{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = compute_shader->Stage(),
                .module = compute_shader->Get(),
                .pName = "main",
            },
        .layout = p_pipeline_layout,
    };

    result = vkCreateComputePipelines(p_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &p_pipeline);
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkCreateComputePipelines");
    }

    ENGINE_LOG_DEBUG("Compute pipeline created");

    // Allocate descriptor sets
    m_descriptor_sets.resize(storage_buffers.size());
    Vector<VkDescriptorSetLayout> layouts(storage_buffers.size(), p_descriptor_set_layout);
    const VkDescriptorSetAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = p_descriptor_pool,
        .descriptorSetCount = static_cast<u32>(storage_buffers.size()),
        .pSetLayouts = layouts.data(),
    };
    result = vkAllocateDescriptorSets(p_device, &alloc_info, m_descriptor_sets.data());
    if (result != VK_SUCCESS) {
        CHECK_VK_RESULT(result, "vkAllocateDescriptorSets");
    }


    // Update descriptor sets
    for (size_t i = 0; i < storage_buffers.size(); ++i) {
        VkDescriptorBufferInfo storage_info{
            .buffer = storage_buffers[i].p_buffer,
            .offset = 0,
            .range = storage_buffer_size,
        };
        VkDescriptorBufferInfo uniform_info{
            .buffer = uniform_buffers[i].p_buffer,
            .offset = 0,
            .range = uniform_buffer_size,
        };

        Vector<VkWriteDescriptorSet> writes{
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptor_sets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &storage_info,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptor_sets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &uniform_info,
            },
        };
        vkUpdateDescriptorSets(p_device, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
    }
}

ComputePipeline::~ComputePipeline() { Destroy(); }

void ComputePipeline::Bind(const VkCommandBuffer command_buffer) const
{
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p_pipeline);
}

void ComputePipeline::BindDescriptors(const VkCommandBuffer command_buffer, const u32 image_index) const
{
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p_pipeline_layout,
                            0, // firstSet
                            1, // descriptorSetCount
                            &m_descriptor_sets[image_index],
                            0,      // dynamicOffsetCount
                            nullptr // pDynamicOffsets
    );
}

u32 ComputePipeline::CalculateWorkGroupCount(const u32 particle_count) const
{
    constexpr u32 local_size_x{256};                               // Matches shader's local_size_x
    return (particle_count + local_size_x - 1) / local_size_x; // Ceiling division
}

void ComputePipeline::Dispatch(VkCommandBuffer command_buffer, const u32 group_count_x, const u32 group_count_y,
                               const u32 group_count_z) const
{
    vkCmdDispatch(command_buffer, group_count_x, group_count_y, group_count_z); // Simplify for 1D dispatch
}

void ComputePipeline::Destroy()
{
    if (p_descriptor_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(p_device, p_descriptor_pool, nullptr);
        p_descriptor_pool = VK_NULL_HANDLE;
    }
    if (p_descriptor_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(p_device, p_descriptor_set_layout, nullptr);
        p_descriptor_set_layout = VK_NULL_HANDLE;
    }
    if (p_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(p_device, p_pipeline, nullptr);
        p_pipeline = VK_NULL_HANDLE;
    }
    if (p_pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(p_device, p_pipeline_layout, nullptr);
        p_pipeline_layout = VK_NULL_HANDLE;
    }

    ENGINE_LOG_DEBUG("Compute pipeline destroyed");
}

} // namespace gouda::vk