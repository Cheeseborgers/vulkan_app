#include "renderers/vulkan/vk_shader.hpp"

#include <ranges>
#include <vector>

#include <vulkan/vulkan.h>

#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Public/resource_limits_c.h"

#include "core/types.hpp"
#include "debug/debug.hpp"
#include "renderers/vulkan/vk_utils.hpp"
#include "utils/filesystem.hpp"

namespace gouda {
namespace vk {

struct Shader {
    std::vector<uint> m_SPIRV;
    VkShaderModule m_shader_module{nullptr};
};

// RAII Wrappers for glslang objects
struct GlslShaderDeleter {
    void operator()(glslang_shader_t *shader) const { glslang_shader_delete(shader); }
};
struct GlslProgramDeleter {
    void operator()(glslang_program_t *program) const { glslang_program_delete(program); }
};

static void PrintShaderSource(std::string_view text)
{
    int line{1};
    std::printf("Shader source --------------------------");
    for (auto line_text : std::views::split(text, '\n')) {

        std::printf("\n(%3i) ", line++);
        std::cout << std::string_view{line_text.begin(), line_text.end()};
    }
    std::printf("--------------------------------------\n");
}

// TODO: Clean this function up
static size_t CompileShader(VkDevice &device, glslang_stage_t stage, const char *p_shader_code, Shader &shader_module)
{
    glslang_input_t input{};
    input.language = GLSLANG_SOURCE_GLSL;
    input.stage = stage;
    input.client = GLSLANG_CLIENT_VULKAN;
    input.client_version = GLSLANG_TARGET_VULKAN_1_1;
    input.target_language = GLSLANG_TARGET_SPV;
    input.target_language_version = GLSLANG_TARGET_SPV_1_3;
    input.code = p_shader_code;
    input.default_version = 100;
    input.default_profile = GLSLANG_NO_PROFILE;
    input.force_default_version_and_profile = false;
    input.forward_compatible = false;
    input.messages = GLSLANG_MSG_DEFAULT_BIT;
    input.resource = glslang_default_resource();

    // Use smart pointers for automatic cleanup
    std::unique_ptr<glslang_shader_t, GlslShaderDeleter> shader{glslang_shader_create(&input)};
    if (!glslang_shader_preprocess(shader.get(), &input)) {
        ENGINE_LOG_ERROR("GLSL Preprocessing failed: info log: {}, debug log: {}",
                         glslang_shader_get_info_log(shader.get()), glslang_shader_get_info_debug_log(shader.get()));
        PrintShaderSource(input.code);

        return 0;
    }

    if (!glslang_shader_parse(shader.get(), &input)) {
        ENGINE_LOG_ERROR("GLSL Parsing failed: info log: {}, debug log: {}", glslang_shader_get_info_log(shader.get()),
                         glslang_shader_get_info_debug_log(shader.get()));
        PrintShaderSource(glslang_shader_get_preprocessed_code(shader.get()));

        return 0;
    }

    std::unique_ptr<glslang_program_t, GlslProgramDeleter> program{glslang_program_create()};
    glslang_program_add_shader(program.get(), shader.get());

    if (!glslang_program_link(program.get(), GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        ENGINE_LOG_ERROR("GLSL Linking failed: info log: {}, debug log: {}", glslang_shader_get_info_log(shader.get()),
                         glslang_shader_get_info_debug_log(shader.get()));
        return 0;
    }

    // Generate SPIR-V
    glslang_program_SPIRV_generate(program.get(), stage);

    shader_module.m_SPIRV.resize(glslang_program_SPIRV_get_size(program.get()));
    glslang_program_SPIRV_get(program.get(), shader_module.m_SPIRV.data());

    // Print any error messages
    if (const char *spirv_messages = glslang_program_SPIRV_get_messages(program.get()); spirv_messages) {
        ENGINE_LOG_ERROR("Shader error message: {}.", spirv_messages);
    }

    VkShaderModuleCreateInfo shader_create_info{};
    shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.pNext = nullptr;
    shader_create_info.flags = 0;
    shader_create_info.codeSize = shader_module.m_SPIRV.size() * sizeof(uint);
    shader_create_info.pCode = reinterpret_cast<const u32 *>(shader_module.m_SPIRV.data());

    VkResult result{vkCreateShaderModule(device, &shader_create_info, nullptr, &shader_module.m_shader_module)};
    CHECK_VK_RESULT(result, "vkCreateShaderModule\n");

    if (result != VK_SUCCESS) {
        return 0;
    }

    return shader_module.m_SPIRV.size();
}

static glslang_stage_t ShaderStageFromFilename(std::string_view file_name)
{
    if (file_name.ends_with(".vs") || file_name.ends_with(".vert")) {
        return GLSLANG_STAGE_VERTEX;
    }

    if (file_name.ends_with(".fs") || file_name.ends_with(".frag")) {
        return GLSLANG_STAGE_FRAGMENT;
    }

    if (file_name.ends_with(".gs") || file_name.ends_with(".geom")) {
        return GLSLANG_STAGE_GEOMETRY;
    }

    if (file_name.ends_with(".cs") || file_name.ends_with(".comp")) {
        return GLSLANG_STAGE_COMPUTE;
    }

    if (file_name.ends_with(".tcs") || file_name.ends_with(".tesc")) {
        return GLSLANG_STAGE_TESSCONTROL;
    }

    if (file_name.ends_with(".tes") || file_name.ends_with(".tese")) {
        return GLSLANG_STAGE_TESSEVALUATION;
    }

    ENGINE_THROW("Unknown shader stage in: {}", file_name);

    return GLSLANG_STAGE_VERTEX;
}

VkShaderModule CreateShaderModuleFromText(VkDevice device_ptr, std::string_view file_name)
{
    const std::string current_path{fs::GetCurrentWorkingDirectory()};
    ENGINE_LOG_INFO("Creating shader from text file: {}", file_name);

    // Read the shader source file
    auto file_result = fs::ReadFile(file_name);
    if (!file_result) {
        switch (file_result.error()) {
            case gouda::fs::Error::FileNotFound:
                ENGINE_LOG_ERROR("Shader file '{}' not found.", file_name);
                ENGINE_THROW("File not found: {}", file_name);
                return nullptr;
            case gouda::fs::Error::FileReadError:
                ENGINE_LOG_ERROR("Failed to read shader file '{}'.", file_name);
                ENGINE_THROW("Failed to read file: {}", file_name);
                return nullptr;
            default:
                ENGINE_LOG_ERROR("Unknown error occurred while reading shader file '{}'.", file_name);
                ENGINE_THROW("Unknown error occurred whilst creating a shader from text: {}", file_name);
                return nullptr;
        }
    }

    std::string shader_source = *file_result;

    // Initialize GLSLang
    glslang_initialize_process();

    // Compile Shader
    Shader shader;
    glslang_stage_t shader_stage{ShaderStageFromFilename(file_name)};
    size_t shader_size{CompileShader(device_ptr, shader_stage, shader_source.c_str(), shader)};

    VkShaderModule shader_module{nullptr};
    if (shader_size > 0) {
        shader_module = shader.m_shader_module;

        // Save compiled SPIR-V binary
        std::string binary_filename{std::string(file_name) + ".spv"};
        auto write_result = fs::WriteBinaryFile(binary_filename, shader.m_SPIRV);
        if (!write_result) {
            ENGINE_LOG_ERROR("Failed to write compiled SPIR-V binary to '{}'", binary_filename);
        }
        else {
            ENGINE_LOG_INFO("Created SPIR-V shader from text file '{}', saved as '{}'", file_name, binary_filename);
        }
    }
    else {
        ENGINE_LOG_ERROR("Shader compilation failed for '{}'.", file_name);
    }

    // Finalize GLSLang
    glslang_finalize_process();

    return shader_module;
}

VkShaderModule CreateShaderModuleFromBinary(VkDevice device_ptr, std::string_view file_name)
{
    VkShaderModule shader_module{nullptr};

    auto file_result = fs::ReadBinaryFile(file_name);
    if (!file_result) {
        ENGINE_LOG_ERROR("Failed to read file '{}' when creating shader from binary. Error: {}", file_name,
                         static_cast<int>(file_result.error()));
        return nullptr;
    }

    const std::vector<char> &shader_code = *file_result;

    if (shader_code.empty()) {
        ENGINE_LOG_ERROR("Shader code is empty when reading from binary: {}", file_name);
        return nullptr;
    }

    VkShaderModuleCreateInfo shader_create_info{};
    shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.pNext = nullptr;
    shader_create_info.flags = 0;
    shader_create_info.codeSize = shader_code.size() * sizeof(char);
    shader_create_info.pCode = reinterpret_cast<const u32 *>(shader_code.data());

    VkResult result{vkCreateShaderModule(device_ptr, &shader_create_info, nullptr, &shader_module)};
    if (result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to create shader module from binary file '{}', VkResult: {}", file_name,
                         VKResultToString(result));
        return nullptr;
    }

    ENGINE_LOG_DEBUG("Successfully created shader from binary: {}", file_name);
    return shader_module;
}

} // namespace vk
} // namespace gouda