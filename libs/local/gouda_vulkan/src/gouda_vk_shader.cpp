#include "gouda_vk_shader.hpp"

#include <ranges>
#include <vector>

#include <vulkan/vulkan.h>

#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Public/resource_limits_c.h"

#include "logger.hpp"

#include "core/types.hpp"
#include "gouda_assert.hpp"
#include "gouda_throw.hpp"
#include "gouda_vk_shader.hpp"
#include "gouda_vk_utils.hpp"
#include "utility/filesystem.hpp"

namespace GoudaVK {

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

    for (auto line_text : std::views::split(text, '\n')) {
        std::printf("\n(%3i) ", line++);
        std::cout << std::string_view{line_text.begin(), line_text.end()};
    }

    std::printf("\n");
}

// TODO: Clean this function up
static std::size_t CompileShader(VkDevice &device, glslang_stage_t stage, const char *p_shader_code,
                                 Shader &shader_module)
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
        // TODO: Log this better
        std::cerr << spirv_messages;
    }

    VkShaderModuleCreateInfo shader_create_info{};
    shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.pNext = nullptr;
    shader_create_info.flags = 0;
    shader_create_info.codeSize = shader_module.m_SPIRV.size() * sizeof(uint);
    shader_create_info.pCode = reinterpret_cast<const u32 *>(shader_module.m_SPIRV.data());

    VkResult result{vkCreateShaderModule(device, &shader_create_info, nullptr, &shader_module.m_shader_module)};
    CHECK_VK_RESULT(result, "vkCreateShaderModule\n");

    ASSERT(result == VK_SUCCESS, "Could not complile shader");

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

    ENGINE_LOG_FATAL("Unknown shader stage in: {}", file_name);
    ENGINE_THROW("Unknown shader stage in: {}", file_name);

    return GLSLANG_STAGE_VERTEX;
}

VkShaderModule CreateShaderModuleFromText(VkDevice device_ptr, std::string_view file_name)
{
    const std::string current_path{Gouda::FS::GetCurrentWorkingDirectory()};
    std::string shader_source;

    ENGINE_LOG_INFO("Creating shader from text file: {}", file_name);

    auto file_result = Gouda::FS::ReadFile(file_name);
    if (file_result) {
        shader_source = *file_result;
    }
    else {
        ENGINE_LOG_ERROR("Failed to read shader from text file: {}", file_result.error());
        ENGINE_THROW("Failed to read shader from text file");
    }

    glslang_initialize_process();

    Shader shader;
    glslang_stage_t shader_stage{ShaderStageFromFilename(file_name)};
    size_t shader_size{CompileShader(device_ptr, shader_stage, shader_source.c_str(), shader)};
    VkShaderModule shader_module{nullptr};

    if (shader_size > 0) {
        shader_module = shader.m_shader_module;

        // TODO: Check this saves in the correct place since we no longer save directly in build dir
        const std::string binary_filename{std::string(file_name) + ".spv"};
        Gouda::FS::WriteBinaryFile(binary_filename, shader.m_SPIRV);

        ENGINE_LOG_INFO("Created SPIRV shader from text file: {}{} as {}{}.spv", current_path, file_name, current_path,
                        binary_filename);
    }

    glslang_finalize_process();

    return shader_module;
}

VkShaderModule CreateShaderModuleFromBinary(VkDevice device_ptr, std::string_view file_name)
{
    VkShaderModule shader_module{nullptr};

    auto file_result = Gouda::FS::ReadBinaryFile(file_name);
    if (file_result) {
        std::vector<char> shader_code = *file_result;

        ASSERT(!shader_code.empty(), "Shader code cannot be empty when reading from binary: {}", file_name);

        VkShaderModuleCreateInfo shader_create_info = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                                       .pNext = nullptr,
                                                       .flags = 0,
                                                       .codeSize = shader_code.size() * sizeof(char),
                                                       .pCode = reinterpret_cast<const u32 *>(shader_code.data())};

        VkResult result{vkCreateShaderModule(device_ptr, &shader_create_info, nullptr, &shader_module)};
        CHECK_VK_RESULT(result, "vkCreateShaderModule\n");

        ASSERT(result == VK_SUCCESS, "Could not create shader from binary file: {}", file_name);

        ENGINE_LOG_DEBUG("Created shader from binary: {}", file_name);
    }
    else {
        ENGINE_LOG_ERROR("Failed to read file when creating shader from binary: {}", file_name);
    }

    return shader_module;
}

} // end namespace