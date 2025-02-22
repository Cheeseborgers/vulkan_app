cmake_minimum_required(VERSION 3.28.1)

project(stb_image LANGUAGES C CXX)

add_library(stb_image INTERFACE)

target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(stb_image INTERFACE STB_IMAGE_IMPLEMENTATION)

# Suppress warnings for stb_image only
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(stb_image INTERFACE -w) # Disable warnings for GCC/Clang
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(stb_image INTERFACE /w) # Disable warnings for MSVC
endif()