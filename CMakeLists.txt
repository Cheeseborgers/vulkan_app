cmake_minimum_required(VERSION 3.28.1)

project(gouda_renderer
    VERSION 0.2.0
    DESCRIPTION "Something"
    HOMEPAGE_URL "https://github.com/Cheeseborgers"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

option(ENABLE_WARNINGS "Enable warnings" ON)
option(COMPILE_SHADERS "Compile shaders to SPIR-V during build" ON)
option(COPY_SHADER_SOURCE "Copy shader source files during build" ON)
option(COPY_TEXTURES "Copy texture image files during build" ON)
option(COPY_AUDIO "Copy audio files during build" ON)
option(COPY_LEVELS "Copy level files during build" ON)
option(USE_CCACHE "Enable ccache" ON)
option(GENERATE_FONTS "Generate msdf fonts on build" ON)
option(COPY_FONTS "Copy msdf fonts on build" ON)

# Logging options
option(ENABLE_ENGINE_LOGGING "Enable engine logging" ON)
option(ENABLE_APP_LOGGING "Enable app logging" ON)
option(ENGINE_LOG_LEVEL_TRACE "Enable engine trace logs" ON)
option(ENGINE_LOG_LEVEL_DEBUG "Enable engine debug logs" ON)
option(ENGINE_LOG_LEVEL_INFO "Enable engine info logs" ON)
option(ENGINE_LOG_LEVEL_WARNING "Enable engine warning logs" ON)
option(ENGINE_LOG_LEVEL_ERROR "Enable engine error logs" ON)
option(ENGINE_LOG_LEVEL_FATAL "Enable engine fatal logs" ON)
option(APP_LOG_LEVEL_TRACE "Enable app trace logs" ON)
option(APP_LOG_LEVEL_DEBUG "Enable app debug logs" ON)
option(APP_LOG_LEVEL_INFO "Enable app info logs" ON)
option(APP_LOG_LEVEL_WARNING "Enable app warning logs" ON)
option(APP_LOG_LEVEL_ERROR "Enable app error logs" ON)
option(APP_LOG_LEVEL_FATAL "Enable app fatal logs" ON)

if(USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)

    if(CCACHE_PROGRAM)
        message("Found ccache at: ${CCACHE_PROGRAM}")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    else()
        message(WARNING "ccache not found, skipping ccache usage.")
    endif()
endif()

find_package(Threads REQUIRED)

add_subdirectory("libs/local/gouda_engine")

# nlohmann_json
add_subdirectory(libs/thirdparty/nlohmann_json)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")

set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build")

add_executable(${PROJECT_NAME}
        include/ui/side_panel.hpp
        src/states/state.cpp
        src/ui/side_panel.cpp
        include/states/states_common.hpp
)

target_sources(${PROJECT_NAME}
    PRIVATE
        src/main.cpp
        src/application.cpp

        src/core/state_stack.cpp
        src/core/settings_manager.cpp

        src/components/animation_component.cpp
        src/components/health_component.cpp

        src/entities/entity.cpp
        src/entities/player.cpp

        src/scenes/scene.cpp

        src/ui/button.cpp
        src/ui/ui_element.cpp
        src/ui/ui_manager.cpp
        src/ui/side_panel.cpp

        src/states/game_state.cpp
        src/states/editor_state.cpp
        src/states/main_menu_state.cpp
        src/states/settings_state.cpp
        src/states/intro_state.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE

    PUBLIC

    # Local libs
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/local/gouda_engine>

    # Third party libs
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/thirdparty/nlohmann_json>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(gouda_engine PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    gouda_engine
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Only enable PCH in Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling precompiled headers (Debug only)")
    add_library(pch INTERFACE)
    target_precompile_headers(pch INTERFACE "include/pch.hpp")

    target_link_libraries(gouda_engine PRIVATE pch)
    target_link_libraries(${PROJECT_NAME} PRIVATE pch)
endif()

# Detect Platform and Required Libraries, adds macros ------------------------------------------------------------------
message(STATUS "${PROJECT_NAME}: Configuring platform-specific stacktrace dependencies...")

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE dbghelp)

elseif(APPLE OR UNIX)
    find_library(UNWIND_LIBRARY unwind)

    if(UNWIND_LIBRARY)
        message(STATUS "Using libunwind: ${UNWIND_LIBRARY}")
        target_link_libraries(${PROJECT_NAME} PRIVATE ${UNWIND_LIBRARY})
        target_compile_definitions(${PROJECT_NAME} PRIVATE USE_LIBUNWIND)
    else()
        find_library(EXECINFO_LIBRARY execinfo)

        if(EXECINFO_LIBRARY)
            message(STATUS "Using execinfo: ${EXECINFO_LIBRARY}")
            target_link_libraries(${PROJECT_NAME} PRIVATE ${EXECINFO_LIBRARY})
        else()
            message(WARNING "Neither libunwind nor execinfo found. Stacktraces may not work.")
        endif()
    endif()
else()
    message(WARNING "${PROJECT_NAME}: Unknown platform. Stacktraces will be disabled.")
endif()

# Compiler flags -------------------------------------------------------------------------------------------------------
function(set_common_compiler_flags target)
    target_compile_options(${target} PRIVATE
        -Wall -Wextra -Werror -Wshadow -Wnon-virtual-dtor -Wformat=2 -Woverloaded-virtual -Wcast-align -Wold-style-cast
        -Wpedantic -Wconversion -Wsign-conversion -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches
        -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wunused-variable -Wuninitialized -g)
endfunction()

# Apply flags based on compiler
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
        message(STATUS "${PROJECT_NAME}: Compiling with Clang (GNU Frontend)")
        set_common_compiler_flags(${PROJECT_NAME})
    elseif(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        message(STATUS "${PROJECT_NAME}: Compiling with Clang (MSVC Frontend)")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "${PROJECT_NAME}: Compiling with GCC")
    set_common_compiler_flags(${PROJECT_NAME})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    message(STATUS "${PROJECT_NAME}: Compiling with Intel C++")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "${PROJECT_NAME}: Compiling with MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
endif()

# Define USE_IMGUI only for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(USE_IMGUI ON CACHE INTERNAL "Use ImGui in engine")
endif()

# Logging compile definitions
if(ENABLE_ENGINE_LOGGING)
    target_compile_definitions(gouda_engine PRIVATE ENABLE_ENGINE_LOGGING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_ENGINE_LOGGING)
endif()

if(ENABLE_APP_LOGGING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_APP_LOGGING)
endif()

if(ENGINE_LOG_LEVEL_TRACE)
    target_compile_definitions(gouda_engine PRIVATE ENGINE_LOG_LEVEL_TRACE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENGINE_LOG_LEVEL_TRACE)
endif()

if(ENGINE_LOG_LEVEL_DEBUG)
    target_compile_definitions(gouda_engine PRIVATE ENGINE_LOG_LEVEL_DEBUG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENGINE_LOG_LEVEL_DEBUG)
endif()

if(ENGINE_LOG_LEVEL_INFO)
    target_compile_definitions(gouda_engine PRIVATE ENGINE_LOG_LEVEL_INFO)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENGINE_LOG_LEVEL_INFO)
endif()

if(ENGINE_LOG_LEVEL_WARNING)
    target_compile_definitions(gouda_engine PRIVATE ENGINE_LOG_LEVEL_WARNING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENGINE_LOG_LEVEL_WARNING)
endif()

if(ENGINE_LOG_LEVEL_ERROR)
    target_compile_definitions(gouda_engine PRIVATE ENGINE_LOG_LEVEL_ERROR)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENGINE_LOG_LEVEL_ERROR)
endif()

if(ENGINE_LOG_LEVEL_FATAL)
    target_compile_definitions(gouda_engine PRIVATE ENGINE_LOG_LEVEL_FATAL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENGINE_LOG_LEVEL_FATAL)
endif()

if(APP_LOG_LEVEL_TRACE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE APP_LOG_LEVEL_TRACE)
endif()

if(APP_LOG_LEVEL_DEBUG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE APP_LOG_LEVEL_DEBUG)
endif()

if(APP_LOG_LEVEL_INFO)
    target_compile_definitions(${PROJECT_NAME} PRIVATE APP_LOG_LEVEL_INFO)
endif()

if(APP_LOG_LEVEL_WARNING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE APP_LOG_LEVEL_WARNING)
endif()

if(APP_LOG_LEVEL_ERROR)
    target_compile_definitions(${PROJECT_NAME} PRIVATE APP_LOG_LEVEL_ERROR)
endif()

if(APP_LOG_LEVEL_FATAL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE APP_LOG_LEVEL_FATAL)
endif()

# Shader compilation and copying ---------------------------------------------------------------------------------------
if(COMPILE_SHADERS)
    find_program(GLSLANG_VALIDATOR glslangValidator)

    if(NOT GLSLANG_VALIDATOR)
        message(FATAL_ERROR " glslangValidator not found! ")
    endif()

    set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/assets/shaders)
    set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets/shaders/compiled)

    file(GLOB SHADER_SOURCES
        ${SHADER_SOURCE_DIR}/*.vert
        ${SHADER_SOURCE_DIR}/*.frag
        ${SHADER_SOURCE_DIR}/*.comp
    )

    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(FILENAME ${SHADER} NAME)
        add_custom_command(
            OUTPUT ${SHADER_OUTPUT_DIR}/${FILENAME}.spv
            COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER} -o ${SHADER_OUTPUT_DIR}/${FILENAME}.spv --target-env vulkan1.2
            DEPENDS ${SHADER}
            COMMENT " Compiling shader ${FILENAME} "
        )
        list(APPEND SPIRV_SHADERS ${SHADER_OUTPUT_DIR}/${FILENAME}.spv)
    endforeach()

    add_custom_target(compile_shaders ALL DEPENDS ${SPIRV_SHADERS})

    add_dependencies(${CMAKE_PROJECT_NAME} compile_shaders)
endif()

# Copy shaders source on build:
if(COPY_SHADER_SOURCE)
    set(SHADER_SOURCE_COPY_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets/shaders/src)
    add_custom_target(copy_shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_SOURCE_DIR} ${SHADER_SOURCE_COPY_DIR}
    )
    add_dependencies(${CMAKE_PROJECT_NAME} copy_shaders)
endif()

# Copy textures on build:
if(COPY_TEXTURES)
    set(TEXTURE_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/assets/textures)
    set(TEXTURE_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets/textures)
    add_custom_target(copy_textures
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${TEXTURE_SOURCE_DIR} ${TEXTURE_DEST_DIR}
    )
    add_dependencies(${CMAKE_PROJECT_NAME} copy_textures)
endif()

# Copy audio on build:
if(COPY_AUDIO)
    set(AUDIO_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/assets/audio)
    set(AUDIO_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets/audio)
    add_custom_target(copy_audio
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${AUDIO_SOURCE_DIR} ${AUDIO_DEST_DIR}
    )
    add_dependencies(${CMAKE_PROJECT_NAME} copy_audio)
endif()

message(STATUS " ${PROJECT_NAME}: Build type: ${CMAKE_BUILD_TYPE} Version: ${PROJECT_VERSION} ")

# Copy audio on levels:
if(COPY_LEVELS)
    set(LEVELS_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/assets/levels)
    set(LEVELS_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets/levels)
    add_custom_target(copy_levels
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${LEVELS_SOURCE_DIR} ${LEVELS_DEST_DIR}
    )
    add_dependencies(${CMAKE_PROJECT_NAME} copy_levels)
endif()

if(GENERATE_FONTS)
    include(GenerateMSDFFont)

    # generate_msdf_font("${CMAKE_SOURCE_DIR}/assets/fonts/Roboto.ttf" "roboto")

    # generate_msdf_font("${CMAKE_SOURCE_DIR}/assets/fonts/OpenSans-Bold.ttf" "opensans")
endif()

# Copy audio on levels:
if(COPY_FONTS)
    set(FONTS_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/assets/fonts)
    set(FONTS_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets/fonts)
    add_custom_target(copy_fonts
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${FONTS_SOURCE_DIR} ${FONTS_DEST_DIR}
    )
    add_dependencies(${CMAKE_PROJECT_NAME} copy_fonts)
endif()

message(STATUS " ${PROJECT_NAME}: Build type: ${CMAKE_BUILD_TYPE} Version: ${PROJECT_VERSION} ")
