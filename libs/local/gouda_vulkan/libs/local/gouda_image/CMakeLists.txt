cmake_minimum_required(VERSION 3.28)
project(gouda_image
    VERSION 0.1.0
    DESCRIPTION "An stb image wrapper"
    HOMEPAGE_URL "https://github.com/Cheeseborgers"
    LANGUAGES C CXX)

add_library(${PROJECT_NAME} STATIC
    src/image.cpp
    src/stb_image.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/thirdparty/stb_image
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

cmake_minimum_required(VERSION 3.28)

message(STATUS "${PROJECT_NAME}: Build type: " ${CMAKE_BUILD_TYPE})
message(STATUS "${PROJECT_NAME}: Version " ${PROJECT_VERSION})