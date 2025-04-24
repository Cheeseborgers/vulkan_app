/**
 * @file renderers/vulkan/vk_shader.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-12
 * @brief Engine module implementation
 */
#include "renderers/vulkan/vk_shader.hpp"

#include <cstring>
#include <ranges>
#include <utility>

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>

#include "core/types.hpp"
#include "debug/debug.hpp"
#include "math/math.hpp"
#include "renderers/vulkan/vk_device.hpp"
#include "renderers/vulkan/vk_utils.hpp"
#include "utils/filesystem.hpp"

namespace std {
// Hash function for std::pair
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2> &p) const
    {
        return std::hash<T1>{}(p.first) ^ std::hash<T2>{}(p.second);
    }
};
}

namespace gouda::vk {

namespace internal {
struct ShaderModule {
    std::vector<u32> m_spirv;
    VkShaderModule p_shader_module{nullptr};
};

// RAII Wrappers for glslang objects
struct GlslShaderDeleter {
    void operator()(glslang_shader_t *shader) const { glslang_shader_delete(shader); }
};
struct GlslProgramDeleter {
    void operator()(glslang_program_t *program) const { glslang_program_delete(program); }
};

[[nodiscard]] constexpr std::string_view vk_format_to_string_view(VkFormat format)
{
    switch (format) {
        // Float formats
        case VK_FORMAT_R32_SFLOAT:
            return "float";
        case VK_FORMAT_R32G32_SFLOAT:
            return "vec2";
        case VK_FORMAT_R32G32B32_SFLOAT:
            return "vec3";
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return "vec4";

        // Integer (signed)
        case VK_FORMAT_R32_SINT:
            return "int";
        case VK_FORMAT_R32G32_SINT:
            return "ivec2";
        case VK_FORMAT_R32G32B32_SINT:
            return "ivec3";
        case VK_FORMAT_R32G32B32A32_SINT:
            return "ivec4";

        // Integer (unsigned)
        case VK_FORMAT_R32_UINT:
            return "uint";
        case VK_FORMAT_R32G32_UINT:
            return "uvec2";
        case VK_FORMAT_R32G32B32_UINT:
            return "uvec3";
        case VK_FORMAT_R32G32B32A32_UINT:
            return "uvec4";

        // Normalized color (treated as float vec4)
        case VK_FORMAT_R8G8B8A8_UNORM:
            return "vec4";
        case VK_FORMAT_B8G8R8A8_UNORM:
            return "vec4";

        // Depth formats
        case VK_FORMAT_D32_SFLOAT:
            return "float (depth)";
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return "depth + stencil";

        default:
            return "unknown";
    }
}

[[nodiscard]] constexpr glslang_stage_t glslang_shader_stage_from_filename(std::string_view file_name) noexcept
{
    if consteval {
        // Compile-time checks for known extensions
        if (file_name.ends_with(".vert"))
            return GLSLANG_STAGE_VERTEX;
        if (file_name.ends_with(".frag"))
            return GLSLANG_STAGE_FRAGMENT;
        if (file_name.ends_with(".geom"))
            return GLSLANG_STAGE_GEOMETRY;
        if (file_name.ends_with(".comp"))
            return GLSLANG_STAGE_COMPUTE;
        if (file_name.ends_with(".tesc"))
            return GLSLANG_STAGE_TESSCONTROL;
        if (file_name.ends_with(".tese"))
            return GLSLANG_STAGE_TESSEVALUATION;
        throw "Unknown shader stage";
    }
    else {
        // Runtime fallback
        if (file_name.ends_with(".vert"))
            return GLSLANG_STAGE_VERTEX;
        if (file_name.ends_with(".frag"))
            return GLSLANG_STAGE_FRAGMENT;
        if (file_name.ends_with(".geom"))
            return GLSLANG_STAGE_GEOMETRY;
        if (file_name.ends_with(".comp"))
            return GLSLANG_STAGE_COMPUTE;
        if (file_name.ends_with(".tesc"))
            return GLSLANG_STAGE_TESSCONTROL;
        if (file_name.ends_with(".tese"))
            return GLSLANG_STAGE_TESSEVALUATION;
        ENGINE_LOG_ERROR("Unknown shader stage in: {}", file_name);
        return GLSLANG_STAGE_VERTEX;
    }
}

[[nodiscard]] VkShaderStageFlagBits vk_shader_stage_from_filename(std::string_view file_name) noexcept
{
    if (file_name.ends_with(".vert") || file_name.ends_with(".vert.spv"))
        return VK_SHADER_STAGE_VERTEX_BIT;
    if (file_name.ends_with(".frag") || file_name.ends_with(".frag.spv"))
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    if (file_name.ends_with(".geom") || file_name.ends_with(".geom.spv"))
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    if (file_name.ends_with(".comp") || file_name.ends_with(".comp.spv"))
        return VK_SHADER_STAGE_COMPUTE_BIT;
    if (file_name.ends_with(".tesc") || file_name.ends_with(".tesc.spv"))
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if (file_name.ends_with(".tese") || file_name.ends_with(".tese.spv"))
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    ENGINE_LOG_ERROR("Unknown Vulkan shader stage for file '{}'", file_name);
    return VK_SHADER_STAGE_ALL;
}

constexpr ShaderFormat infer_shader_format_from_extension(std::string_view file_name) noexcept
{
    if (file_name.ends_with(".spv"))
        return ShaderFormat::Binary;
    if (file_name.ends_with(".vert") || file_name.ends_with(".frag") || file_name.ends_with(".geom") ||
        file_name.ends_with(".comp") || file_name.ends_with(".tesc") || file_name.ends_with(".tese"))
        return ShaderFormat::Text;
    ENGINE_LOG_ERROR("Cannot infer shader type from file extension: '{}'", file_name);
    return ShaderFormat::Text;
}

VkFormat spirv_type_to_vk_format(const spirv_cross::SPIRType &type)
{
    if (type.basetype == spirv_cross::SPIRType::Float) {
        if (type.vecsize == 1)
            return VK_FORMAT_R32_SFLOAT;
        if (type.vecsize == 2)
            return VK_FORMAT_R32G32_SFLOAT;
        if (type.vecsize == 3)
            return VK_FORMAT_R32G32B32_SFLOAT;
        if (type.vecsize == 4)
            return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    else if (type.basetype == spirv_cross::SPIRType::Int) {
        if (type.vecsize == 1)
            return VK_FORMAT_R32_SINT;
        if (type.vecsize == 2)
            return VK_FORMAT_R32G32_SINT;
        if (type.vecsize == 3)
            return VK_FORMAT_R32G32B32_SINT;
        if (type.vecsize == 4)
            return VK_FORMAT_R32G32B32A32_SINT;
    }
    else if (type.basetype == spirv_cross::SPIRType::UInt) {
        if (type.vecsize == 1)
            return VK_FORMAT_R32_UINT;
        if (type.vecsize == 2)
            return VK_FORMAT_R32G32_UINT;
        if (type.vecsize == 3)
            return VK_FORMAT_R32G32B32_UINT;
        if (type.vecsize == 4)
            return VK_FORMAT_R32G32B32A32_UINT;
    }
    ENGINE_LOG_WARNING("Unsupported SPIR-V type for vertex input");
    return VK_FORMAT_UNDEFINED;
}

void print_shader_source(std::string_view text) noexcept
{
    int line{1};
    ENGINE_LOG_INFO("Shader source --------------------------");
    for (auto line_text : std::ranges::split_view(text, '\n')) {
        ENGINE_LOG_INFO("({:3}) {}", line++, std::string_view(line_text.begin(), line_text.end()));
    }
    ENGINE_LOG_INFO("--------------------------------------");
}

ShaderReflection reflect_shader(const std::vector<u32> &spirv, VkShaderStageFlagBits stage)
{
    ShaderReflection reflection;
    reflection.entry_point = "main";

    try {
        spirv_cross::Compiler compiler(spirv);
        spirv_cross::ShaderResources resources{compiler.get_shader_resources()};

        // Get the shaders entry
        auto entry_points = compiler.get_entry_points_and_stages();
        if (!entry_points.empty()) {
            reflection.entry_point = entry_points[0].name;
        }

        // Helper lambda to add a descriptor binding
        auto add_binding = [&](const spirv_cross::Resource &resource, VkDescriptorType type) {
            ShaderDescriptorBinding binding{};
            binding.name = resource.name;
            binding.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            binding.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            binding.type = type;
            binding.stage_flags = stage;
            const auto &type_info = compiler.get_type(resource.type_id);
            binding.count = type_info.array.empty() ? 1 : type_info.array[0];

            // Basic validation
            if (binding.set == ~0u || binding.binding == ~0u) {
                ENGINE_LOG_WARNING("Resource '{}' in stage {} missing set or binding decoration",
                                   binding.name.empty() ? "(unnamed)" : binding.name,
                                   vk_shader_stage_as_string_view(stage));
                return;
            }

            // Temporary: Only include set 0 bindings until GraphicsPipeline supports multiple sets
            if (binding.set != 0) {
                ENGINE_LOG_WARNING("Skipping descriptor binding '{}' in stage {}: set={} (only set=0 supported)",
                                   binding.name.empty() ? "(unnamed)" : binding.name,
                                   vk_shader_stage_as_string_view(stage), binding.set);
                return;
            }

            // Validate array size for samplers
            if (binding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && binding.count > MAX_TEXTURES) {
                ENGINE_LOG_WARNING("Descriptor binding '{}' in stage {} has count={} exceeding MAX_TEXTURES={}",
                                   binding.name.empty() ? "(unnamed)" : binding.name,
                                   vk_shader_stage_as_string_view(stage), binding.count, MAX_TEXTURES);
                binding.count = MAX_TEXTURES; // Truncate to MAX_TEXTURES
            }

            ENGINE_LOG_DEBUG("Descriptor binding: name='{}', set={}, binding={}, type={}, count={}, stage={}",
                             binding.name.empty() ? "(unnamed)" : binding.name, binding.set, binding.binding,
                             vk_descriptor_type_to_string_view(binding.type), binding.count,
                             vk_shader_stage_as_string_view(stage));

            reflection.descriptor_bindings.push_back(binding);
        };

        // Uniform Buffers
        for (const auto &ubo : resources.uniform_buffers) {
            add_binding(ubo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }

        // Sampled Images
        for (const auto &sampler : resources.sampled_images) {
            add_binding(sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }

        // Storage Buffers
        for (const auto &sbo : resources.storage_buffers) {
            add_binding(sbo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        }

        // Storage Images
        for (const auto &image : resources.storage_images) {
            add_binding(image, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        }

        // Separate Samplers
        for (const auto &sampler : resources.separate_samplers) {
            add_binding(sampler, VK_DESCRIPTOR_TYPE_SAMPLER);
        }

        // Separate Images
        for (const auto &image : resources.separate_images) {
            add_binding(image, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
        }

        // Input Attachments
        for (const auto &attachment : resources.subpass_inputs) {
            add_binding(attachment, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
        }

        // Push Constants
        for (const auto &pc : resources.push_constant_buffers) {
            const spirv_cross::SPIRType &type = compiler.get_type(pc.base_type_id);
            size_t size{compiler.get_declared_struct_size(type)};
            u32 offset{compiler.get_decoration(pc.id, spv::DecorationOffset)};
            ENGINE_LOG_DEBUG("Push constant: stage={}, offset={}, size={}", vk_shader_stage_as_string_view(stage),
                             offset, size);
            ShaderPushConstantRange range{};
            range.offset = offset;
            range.size = static_cast<u32>(size);
            range.stage_flags = stage;
            reflection.push_constants.push_back(range);
        }

        // Specialization Constants
        std::unordered_set<u32> seen_constant_ids;
        for (const auto &spec : compiler.get_specialization_constants()) {
            if (!seen_constant_ids.insert(spec.constant_id).second) {
                ENGINE_LOG_ERROR("Duplicate specialization constant_id {} in stage {}", spec.constant_id,
                                 vk_shader_stage_as_string_view(stage));
            }

            ShaderSpecializationConstant constant{};
            constant.constant_id = spec.constant_id;
            constant.name = compiler.get_name(spec.id);
            spirv_cross::SPIRConstant constant_value = compiler.get_constant(spec.id);
            const auto &type = compiler.get_type(constant_value.constant_type); // Use constant_type
            constant.type = type.basetype == spirv_cross::SPIRType::Int     ? VkConstantType::VK_CONSTANT_TYPE_INT
                            : type.basetype == spirv_cross::SPIRType::Float ? VkConstantType::VK_CONSTANT_TYPE_FLOAT
                                                                            : VkConstantType::VK_CONSTANT_TYPE_UNKNOWN;

            if (constant.type != VkConstantType::VK_CONSTANT_TYPE_UNKNOWN) {
                constant.default_value.resize(4); // 4 bytes for scalar int or float
                if (constant.type == VkConstantType::VK_CONSTANT_TYPE_INT) {
                    int32_t value = constant_value.scalar_i32();
                    std::memcpy(constant.default_value.data(), &value, 4);
                }
                else if (constant.type == VkConstantType::VK_CONSTANT_TYPE_FLOAT) {
                    float value = constant_value.scalar_f32();
                    std::memcpy(constant.default_value.data(), &value, 4);
                }
            }

            reflection.specialization_constants.push_back(constant);
        }

        // Vertex Inputs
        if (stage == VK_SHADER_STAGE_VERTEX_BIT) {
            for (const auto &input : resources.stage_inputs) {
                ShaderVertexInput vertex_input{};
                vertex_input.location = compiler.get_decoration(input.id, spv::DecorationLocation);
                vertex_input.name = input.name;
                const auto &type = compiler.get_type(input.type_id);
                vertex_input.format = spirv_type_to_vk_format(type);

                vertex_input.input_rate = vertex_input.name.find("instance_") == 0 ? VK_VERTEX_INPUT_RATE_INSTANCE
                                                                                   : VK_VERTEX_INPUT_RATE_VERTEX;
                ENGINE_LOG_DEBUG("Vertex input: name={}, location={}, format={}, input_rate={}", vertex_input.name,
                                 vertex_input.location, vk_format_to_string_view(vertex_input.format),
                                 vertex_input.input_rate == VK_VERTEX_INPUT_RATE_VERTEX ? "VERTEX" : "INSTANCE");

                reflection.vertex_inputs.push_back(vertex_input);
            }
        }
    }
    catch (const spirv_cross::CompilerError &e) {
        ENGINE_LOG_ERROR("SPIR-V reflection failed for stage {}: {}", vk_shader_stage_as_string_view(stage), e.what());
    }

    return reflection;
}

size_t compile_shader(VkDevice device, glslang_stage_t stage, std::string_view shader_code,
                      internal::ShaderModule &shader_module) noexcept
{
    ENGINE_PROFILE_SCOPE("Compile Shader");

    glslang_input_t input{};
    input.language = GLSLANG_SOURCE_GLSL;
    input.stage = stage;
    input.client = GLSLANG_CLIENT_VULKAN;
    input.client_version = GLSLANG_TARGET_VULKAN_1_1;
    input.target_language = GLSLANG_TARGET_SPV;
    input.target_language_version = GLSLANG_TARGET_SPV_1_3;
    input.code = shader_code.data();
    input.default_version = 100;
    input.default_profile = GLSLANG_NO_PROFILE;
    input.messages = GLSLANG_MSG_DEFAULT_BIT;
    input.resource = glslang_default_resource();

    std::unique_ptr<glslang_shader_t, internal::GlslShaderDeleter> shader{glslang_shader_create(&input)};
    if (!glslang_shader_preprocess(shader.get(), &input)) {
        ENGINE_LOG_ERROR("GLSL Preprocessing failed: info log: {}, debug log: {}",
                         glslang_shader_get_info_log(shader.get()), glslang_shader_get_info_debug_log(shader.get()));
        print_shader_source(shader_code);
        return 0;
    }

    if (!glslang_shader_parse(shader.get(), &input)) {
        ENGINE_LOG_ERROR("GLSL Parsing failed: info log: {}, debug log: {}", glslang_shader_get_info_log(shader.get()),
                         glslang_shader_get_info_debug_log(shader.get()));
        print_shader_source(glslang_shader_get_preprocessed_code(shader.get()));
        return 0;
    }

    std::unique_ptr<glslang_program_t, internal::GlslProgramDeleter> program{glslang_program_create()};
    glslang_program_add_shader(program.get(), shader.get());

    if (!glslang_program_link(program.get(), GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        ENGINE_LOG_ERROR("GLSL Linking failed: info log: {}, debug log: {}", glslang_shader_get_info_log(shader.get()),
                         glslang_shader_get_info_debug_log(shader.get()));
        return 0;
    }

    glslang_program_SPIRV_generate(program.get(), stage);
    shader_module.m_spirv.resize(glslang_program_SPIRV_get_size(program.get()));
    glslang_program_SPIRV_get(program.get(), shader_module.m_spirv.data());

    if (const char *spirv_messages = glslang_program_SPIRV_get_messages(program.get())) {
        ENGINE_LOG_ERROR("Shader error message: {}", spirv_messages);
    }

    VkShaderModuleCreateInfo shader_create_info{};
    shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.codeSize = shader_module.m_spirv.size() * sizeof(u32);
    shader_create_info.pCode = shader_module.m_spirv.data();

    VkResult result{vkCreateShaderModule(device, &shader_create_info, nullptr, &shader_module.p_shader_module)};
    if (result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("vkCreateShaderModule failed: {}", vk_result_to_string(result));
        return 0;
    }

    return shader_module.m_spirv.size();
}

} // namespace internal

[[nodiscard]] constexpr std::string_view vk_shader_stage_as_string_view(VkShaderStageFlagBits stage) noexcept
{
    switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return "Vertex";
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return "Fragment";
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return "Geometry";
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return "Compute";
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            return "Tessellation control";
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return "Tessellation evaluation";
        case VK_SHADER_STAGE_ALL:
            return "All";
        default:
            return "Unknown";
    }
}

[[nodiscard]] constexpr std::string_view shader_error_to_string_view(ShaderError error) noexcept
{
    switch (error) {
        case ShaderError::FileNotFound:
            return "File not found";
        case ShaderError::FileReadError:
            return "File read error";
        case ShaderError::CompilationFailed:
            return "Shader compilation failed";
        case ShaderError::VulkanError:
            return "Vulkan API error";
        case ShaderError::Unknown:
            return "Unknown shader error";
        default:
            return "Invalid ShaderError";
    }
}

// Shader -----------------------------------------------------------------------------------
Shader::Shader(Device &device, std::string_view file_name, ShaderFormat format)
    : m_device(device), m_format(format), m_stage(internal::vk_shader_stage_from_filename(file_name))
{
    auto result = (format == ShaderFormat::Binary) ? CreateShaderModuleFromBinary(file_name)
                                                   : CreateShaderModuleFromText(file_name);
    if (!result) {
        ENGINE_LOG_ERROR("Failed to create shader module for '{}': {}", file_name,
                         shader_error_to_string_view(result.error()));
        ENGINE_THROW("Shader creation failed");
    }
    p_module = *result;
}

Shader::Shader(Device &device, std::string_view file_name)
    : Shader(device, file_name, internal::infer_shader_format_from_extension(file_name))
{
}

Shader::Shader(Shader &&other) noexcept
    : m_device(other.m_device),
      p_module(std::exchange(other.p_module, VK_NULL_HANDLE)),
      m_format(other.m_format),
      m_stage(other.m_stage)
{
}

Shader &Shader::operator=(Shader &&other) noexcept
{
    if (this != &other) {
        Destroy();
        m_device = other.m_device;
        p_module = std::exchange(other.p_module, VK_NULL_HANDLE);
        m_format = other.m_format;
        m_stage = other.m_stage;
    }
    return *this;
}

Shader::~Shader() { Destroy(); }

// Private functions ------------------------------------------------------------------------
Expect<VkShaderModule, ShaderError> Shader::CreateShaderModuleFromBinary(std::string_view file_name) noexcept
{
    ENGINE_PROFILE_SCOPE("Create shader from binary");

    auto file_result = fs::ReadBinaryFile(file_name);
    if (!file_result) {
        ENGINE_LOG_ERROR("Failed to read binary file '{}': {}", file_name, error_to_string(file_result.error()));
        return std::unexpected(file_result.error() == fs::Error::FileNotFound ? ShaderError::FileNotFound
                                                                              : ShaderError::FileReadError);
    }

    std::span<const std::byte> shader_code{*file_result};
    if (shader_code.empty()) {
        ENGINE_LOG_ERROR("Shader code is empty: {}", file_name);
        return std::unexpected(ShaderError::FileReadError);
    }

    // Reinterpret byte data to u32 vector (SPIR-V format)
    if (shader_code.size() % sizeof(u32) != 0) {
        ENGINE_LOG_ERROR("SPIR-V binary size is not aligned: {}", file_name);
        return std::unexpected(ShaderError::FileReadError);
    }

    std::vector<u32> spirv(reinterpret_cast<const u32 *>(shader_code.data()),
                           reinterpret_cast<const u32 *>(shader_code.data() + shader_code.size()));

    VkShaderModuleCreateInfo shader_create_info{};
    shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.codeSize = shader_code.size();
    shader_create_info.pCode = reinterpret_cast<const u32 *>(shader_code.data());

    VkShaderModule shader_module{VK_NULL_HANDLE};
    VkResult result{vkCreateShaderModule(m_device.GetDevice(), &shader_create_info, nullptr, &shader_module)};
    if (result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to create shader module from '{}': {}", file_name, vk_result_to_string(result));
        return std::unexpected(ShaderError::VulkanError);
    }

    m_reflection = internal::reflect_shader(spirv, m_stage);

    ENGINE_LOG_DEBUG("Successfully created {} shader from binary: {}", vk_shader_stage_as_string_view(m_stage),
                     file_name);
    return shader_module;
}

Expect<VkShaderModule, ShaderError> Shader::CreateShaderModuleFromText(std::string_view file_name) noexcept
{
    ENGINE_PROFILE_SCOPE("Create shader from text");

    ENGINE_LOG_INFO("Creating shader from text file: {}", file_name);

    auto file_result = fs::ReadFile(file_name);
    if (!file_result) {
        ENGINE_LOG_ERROR("Failed to read shader file '{}': {}", file_name, error_to_string(file_result.error()));
        return std::unexpected(file_result.error() == fs::Error::FileNotFound ? ShaderError::FileNotFound
                                                                              : ShaderError::FileReadError);
    }

    glslang_initialize_process();
    internal::ShaderModule shader;
    glslang_stage_t shader_stage{internal::glslang_shader_stage_from_filename(file_name)};
    size_t shader_size{internal::compile_shader(m_device.GetDevice(), shader_stage, *file_result, shader)};

    if (shader_size == 0) {
        glslang_finalize_process();
        ENGINE_LOG_ERROR("Shader compilation failed for '{}'", file_name);
        return std::unexpected(ShaderError::CompilationFailed);
    }

    m_reflection = internal::reflect_shader(shader.m_spirv, m_stage);

    VkShaderModule shader_module{shader.p_shader_module};
    std::string binary_filename{std::string(file_name) + ".spv"};
    if (!fs::WriteBinaryFile(binary_filename, shader.m_spirv)) {
        ENGINE_LOG_ERROR("Failed to write SPIR-V binary to '{}'", binary_filename);
    }
    else {
        ENGINE_LOG_INFO("Created SPIR-V {} shader from '{}', saved as '{}'", vk_shader_stage_as_string_view(m_stage),
                        file_name, binary_filename);
    }

    glslang_finalize_process();
    return shader_module;
}

void Shader::Destroy() noexcept
{
    if (p_module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_device.GetDevice(), p_module, nullptr);
        p_module = VK_NULL_HANDLE;
    }
}

} // namespace gouda::vk