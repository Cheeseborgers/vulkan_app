cmake_minimum_required(VERSION 3.28)

project(gouda_logger
    VERSION 0.1.0
    DESCRIPTION "A simple header only logging lib"
    HOMEPAGE_URL "https://github.com/Cheeseborgers"
    LANGUAGES C CXX)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23) # Enforce C++23

message(STATUS "${PROJECT_NAME}: Build type: " ${CMAKE_BUILD_TYPE})
message(STATUS "${PROJECT_NAME}: Version " ${PROJECT_VERSION})