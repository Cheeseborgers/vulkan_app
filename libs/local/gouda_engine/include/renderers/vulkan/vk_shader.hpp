#pragma once
/**
 * @file renderers/vulkan/vk_shader.hpp
 * @brief Vulkan shader module abstraction and reflection support.
 *
 * @author GoudaCheeseburgers
 * @date 2025-03-12
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include <string_view>
#include <vector>

#include <vulkan/vulkan.h>

#include "core/types.hpp"

namespace gouda::vk {

/// Shader file format (binary or text/GLSL)
enum class ShaderFormat : u8 { Binary, Text };

/// Possible errors during shader loading or compilation
enum class ShaderError : u8 { FileNotFound, FileReadError, CompilationFailed, VulkanError, Unknown };

/// Type of specialization constant used in shaders
enum class VkConstantType : u8 { VK_CONSTANT_TYPE_INT, VK_CONSTANT_TYPE_FLOAT, VK_CONSTANT_TYPE_UNKNOWN };

/// Represents a descriptor binding within a shader (descriptor set, binding, type, etc.)
struct ShaderDescriptorBinding {
    std::string name;               ///< Binding name
    u32 set;                        ///< Descriptor set index
    u32 binding;                    ///< Binding number
    VkDescriptorType type;          ///< Descriptor type (e.g., uniform buffer, sampler)
    VkShaderStageFlags stage_flags; ///< Shader stages using this binding
    u32 count;                      ///< Number of elements (e.g., array size)
};

/// Represents a push constant range in a shader
struct ShaderPushConstantRange {
    u32 offset;                     ///< Byte offset of the range
    u32 size;                       ///< Size of the range in bytes
    VkShaderStageFlags stage_flags; ///< Shader stages using this push constant
};

/// Represents a specialization constant in a shader
struct ShaderSpecializationConstant {
    u32 constant_id;               ///< Constant ID used in shader
    std::string name;              ///< Optional human-readable name
    VkConstantType type;           ///< Type of the constant
    std::vector<u8> default_value; ///< Default value blob
};

/// Vertex input attribute as described in the shader
struct ShaderVertexInput {
    u32 location;                 ///< Location number
    std::string name;             ///< Attribute name
    VkFormat format;              ///< Vulkan format (e.g., vec3 = VK_FORMAT_R32G32B32_SFLOAT)
    VkVertexInputRate input_rate; ///< Vertex or instance rate
};

/// Full shader reflection data: resource bindings, push constants, etc.
struct ShaderReflection {
    std::vector<ShaderDescriptorBinding> descriptor_bindings;           ///< Descriptor set bindings
    std::vector<ShaderPushConstantRange> push_constants;                ///< Push constant ranges
    std::vector<ShaderSpecializationConstant> specialization_constants; ///< Specialization constants
    std::vector<ShaderVertexInput> vertex_inputs;                       ///< Vertex input attributes
    std::string entry_point;
};

class Device;

/**
 * @class Shader
 * @brief Wrapper around Vulkan shader modules with reflection data support.
 */
class Shader {
public:
    /**
     * @brief Constructs a shader from file with explicit format.
     * @param device Vulkan device reference.
     * @param file_name Path to the shader file.
     * @param format Format of the shader (binary or text).
     */
    Shader(Device &device, std::string_view file_name, ShaderFormat format);

    /**
     * @brief Constructs a shader from file, auto-detecting format.
     * @param device Vulkan device reference.
     * @param file_name Path to the shader file.
     */
    Shader(Device &device, std::string_view file_name);

    /// Deleted copy constructor
    Shader(const Shader &) = delete;

    /// Deleted copy assignment
    Shader &operator=(const Shader &) = delete;

    /// Move constructor
    Shader(Shader &&other) noexcept;

    /// Move assignment
    Shader &operator=(Shader &&other) noexcept;

    /// Destructor, destroys the shader module
    ~Shader();

    /**
     * @brief Gets the Vulkan shader module handle.
     * @return VkShaderModule handle.
     */
    [[nodiscard]] constexpr VkShaderModule Get() const noexcept { return p_module; }

    /**
     * @brief Gets the shader stage flag (e.g., vertex, fragment).
     * @return Shader stage.
     */
    [[nodiscard]] constexpr VkShaderStageFlagBits Stage() const noexcept { return m_stage; }

    /**
     * @brief Returns the shader reflection information.
     * @return Constant reference to ShaderReflection.
     */
    [[nodiscard]] constexpr const ShaderReflection &Reflection() const noexcept { return m_reflection; }

    /**
     * @brief Indicates whether the shader was successfully loaded.
     * @return true if shader is valid; false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept { return p_module != VK_NULL_HANDLE; }

private:
    /**
     * @brief Creates a shader module from SPIR-V binary.
     * @param file_name Path to the binary file.
     * @return Expected VkShaderModule or ShaderError.
     */
    [[nodiscard]] Expect<VkShaderModule, ShaderError> CreateShaderModuleFromBinary(std::string_view file_name) noexcept;

    /**
     * @brief Creates a shader module from GLSL text.
     * @param file_name Path to the text file.
     * @return Expected VkShaderModule or ShaderError.
     */
    [[nodiscard]] Expect<VkShaderModule, ShaderError> CreateShaderModuleFromText(std::string_view file_name) noexcept;

    /// Destroys the shader module if valid.
    void Destroy() noexcept;

private:
    Device &m_device;                ///< Vulkan device used for creation
    VkShaderModule p_module;         ///< Vulkan shader module handle
    ShaderFormat m_format{};         ///< Original shader format (binary or text)
    VkShaderStageFlagBits m_stage{}; ///< Shader stage (vertex, fragment, etc.)
    ShaderReflection m_reflection{}; ///< Parsed shader reflection data
};

/**
 * @brief Converts a shader stage enum to a human-readable string view.
 * @param stage Shader stage flag.
 * @return String view representation.
 */
[[nodiscard]] constexpr std::string_view vk_shader_stage_as_string_view(VkShaderStageFlagBits stage) noexcept;

/**
 * @brief Converts a VkFormat enum to a string view.
 * @param format Vulkan format enum.
 * @return String view of the format.
 */
[[nodiscard]] constexpr std::string_view vk_format_to_string_view(VkFormat format) noexcept
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

/**
 * @brief Converts a VkDescriptorType to a human-readable string view.
 * @param type Vulkan descriptor type.
 * @return String view describing the descriptor type.
 */
[[nodiscard]] constexpr std::string_view vk_descriptor_type_to_string_view(VkDescriptorType type) noexcept
{
    switch (type) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
            return "Sampler";
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            return "Combined Image Sampler";
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return "Sampled Image";
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return "Storage Image";
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return "Uniform Texel Buffer";
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return "Storage Texel Buffer";
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return "Uniform Buffer";
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return "Storage Buffer";
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return "Uniform Buffer (Dynamic)";
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return "Storage Buffer (Dynamic)";
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return "Input Attachment";
        case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
            return "Inline Uniform Block";
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            return "Acceleration Structure (KHR)";
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
            return "Acceleration Structure (NV)";
        case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
            return "Mutable Descriptor (EXT)";
        case VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM:
            return "Sample Weight Image (QCOM)";
        case VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM:
            return "Block Match Image (QCOM)";
        default:
            return "Unknown Descriptor Type";
    }
}

/**
 * @brief Converts a ShaderError enum to a human-readable string view.
 * @param error ShaderError value.
 * @return Descriptive string view of the error.
 */
[[nodiscard]] constexpr std::string_view shader_error_to_string_view(ShaderError error) noexcept;

} // namespace gouda::vk
