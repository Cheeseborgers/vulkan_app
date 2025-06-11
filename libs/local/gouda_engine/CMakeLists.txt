cmake_minimum_required(VERSION 3.28.1)

project(gouda_engine
        VERSION 0.1.0
        DESCRIPTION "Gouda Engine"
        HOMEPAGE_URL "https://github.com/Cheeseborgers"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

find_package(Vulkan REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(Threads REQUIRED)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")
add_subdirectory("libs/thirdparty/glfw")

# Audio
add_subdirectory(libs/thirdparty/openAL)
add_subdirectory(libs/thirdparty/libsndfile)

# ImGUI
set(USE_IMGUI ON)
add_subdirectory(libs/thirdparty/imgui)

# pcg random
add_subdirectory(libs/thirdparty/pcg)

# stb_image
add_subdirectory(libs/thirdparty/stb_image)

# Disable unnecessary features and avoid errors
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_CPP ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_GLSL ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(libs/thirdparty/SPIRV-Cross)

set(PROJECT_SOURCES
        src/audio/audio_common.cpp
        src/audio/audio_manager.cpp
        src/audio/music_track.cpp
        src/audio/sound_effect.cpp

        src/backends/common.cpp
        src/backends/input_handler.cpp
        src/backends/glfw/glfw_backend.cpp
        src/backends/glfw/glfw_window.cpp

        src/cameras/camera.cpp
        src/cameras/orthographic_camera.cpp
        src/cameras/perspective_camera.cpp

        src/debug/assert.cpp
        src/debug/profiler.cpp
        src/debug/stacktrace.cpp

        src/renderers/text.cpp
        src/renderers/render_data.cpp

        src/renderers/vulkan/gouda_vk_wrapper.cpp
        src/renderers/vulkan/vk_buffer.cpp
        src/renderers/vulkan/vk_buffer_manager.cpp
        src/renderers/vulkan/vk_command_buffer_manager.cpp
        src/renderers/vulkan/vk_compute_pipeline.cpp
        src/renderers/vulkan/vk_depth_resources.cpp
        src/renderers/vulkan/vk_device.cpp
        src/renderers/vulkan/vk_fence.cpp
        src/renderers/vulkan/vk_font_manager.cpp
        src/renderers/vulkan/vk_graphics_pipeline.cpp
        src/renderers/vulkan/vk_instance.cpp
        src/renderers/vulkan/vk_renderer.cpp
        src/renderers/vulkan/vk_semaphore.cpp
        src/renderers/vulkan/vk_swapchain.cpp
        src/renderers/vulkan/vk_texture_manager.cpp
        src/renderers/vulkan/vk_utils.cpp
        src/renderers/vulkan/vk_queue.cpp
        src/renderers/vulkan/vk_shader.cpp
        src/renderers/vulkan/vk_texture.cpp

        src/math/random.cpp
        src/math/matrix4x4.cpp
        src/math/transform.cpp
        src/math/vector.cpp
        src/math/quaternion.cpp

        src/utils/filesystem.cpp
        src/utils/image.cpp
        include/math/easing.hpp

)

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME}
        PUBLIC

        # Thirdparty libs
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/thirdparty/openAL>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/thirdparty/libsndfile>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/thirdparty/imgui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/thirdparty/pcg>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/thirdparty/stb_image>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/thirdparty/SPIRV-Cross>

        # Project includes
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        glfw
        Vulkan::Vulkan
        stb_image
        OpenAL
        sndfile
        imgui
        pcg

        # Shaders
        spirv-cross-core
        spirv-cross-glsl
        glslang::glslang
        glslang::SPIRV
        glslang::glslang-default-resource-limits
)

if (USE_IMGUI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_IMGUI)
endif ()

# Compile options --------------------------------------------
target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Werror -Wshadow -Wnon-virtual-dtor -Wformat=2 -Woverloaded-virtual -Wcast-align -Wold-style-cast
        -Wpedantic -Wconversion -Wsign-conversion -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches
        -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wunused-variable -Wuninitialized -g)

message(STATUS "${PROJECT_NAME}: Build type: " ${CMAKE_BUILD_TYPE})
message(STATUS "${PROJECT_NAME}: Version " ${PROJECT_VERSION})