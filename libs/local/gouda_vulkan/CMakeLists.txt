cmake_minimum_required(VERSION 3.28.1)

project(gouda_vulkan
    VERSION 0.1.0
    DESCRIPTION "Vulkan core"
    HOMEPAGE_URL "https://github.com/Cheeseborgers"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Vulkan
find_package(Vulkan REQUIRED)

find_package(glslang CONFIG REQUIRED)

find_package(Threads REQUIRED)

# Audio
add_subdirectory(libs/thirdparty/openAL)
add_subdirectory(libs/thirdparty/libsndfile)

add_subdirectory(libs/thirdparty/imgui)

# pcg random
add_subdirectory(libs/thirdparty/pcg)

# Add stb_image for texture creation
add_subdirectory(libs/thirdparty/stb_image)

# add_subdirectory(libs/local/gouda_image)
set(PROJECT_SOURCES
    src/audio/audio_common.cpp
    src/audio/audio_manager.cpp
    src/audio/music_track.cpp
    src/audio/sound_effect.cpp
    src/backends/common.cpp
    src/backends/glfw/window.cpp
    src/cameras/camera.cpp
    src/cameras/orthographic_camera.cpp
    src/cameras/perspective_camera.cpp
    src/core/input_system.cpp
    src/debug/assert.cpp
    src/gouda_vk_allocated_buffer.cpp
    src/gouda_vk_core.cpp
    src/gouda_vk_device.cpp
    src/gouda_vk_graphics_pipeline.cpp
    src/gouda_vk_instance.cpp
    src/gouda_vk_utils.cpp
    src/gouda_vk_wrapper.cpp
    src/gouda_vk_queue.cpp
    src/gouda_vk_shader.cpp
    src/gouda_vk_simple_mesh.cpp
    src/gouda_vk_texture.cpp
    src/math/random.cpp
    src/math/matrix4x4.cpp
    src/math/transform.cpp
    src/math/vector.cpp
    src/math/quaternion.cpp
    src/utility/filesystem.cpp
)

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC

    # Local libs
    # $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/local/gouda_image>

    # Thirdpart libs
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/thirdparty/openAL>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/thirdparty/libsndfile>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/thirdparty/imgui>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/thirdparty/pcg>

    # Project includes
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    gouda_logger

    # gouda_image
    stb_image

    # Sound
    OpenAL
    sndfile

    imgui

    pcg

    Vulkan::Vulkan

    # Shaders
    glslang::glslang
    glslang::SPIRV
    glslang::glslang-default-resource-limits
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:GOUDA_DEBUG=1>
    $<$<CONFIG:MinSizeRel>:GOUDA_DEBUG=0>
    $<$<CONFIG:Release>:GOUDA_DEBUG=0>
    $<$<CONFIG:RelWithDebInfo>:GOUDA_DEBUG=0>
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:GOUDA_DEBUG=0>
)

# Compile options --------------------------------------------
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wnon-virtual-dtor -pedantic
    -Wold-style-cast -Wpedantic -Wconversion -Wsign-conversion
    -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches
    -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion
    -Wunused-variable -Wuninitialized)

message(STATUS "${PROJECT_NAME}: Build type: " ${CMAKE_BUILD_TYPE})
message(STATUS "${PROJECT_NAME}: Version " ${PROJECT_VERSION})