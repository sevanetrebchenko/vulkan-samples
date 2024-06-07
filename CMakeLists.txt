
cmake_minimum_required(VERSION 3.3)
project(vulkan_samples VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/scripts" ${CMAKE_MODULE_PATH})

include(add_subdirectories)
include(add_project)

add_subdirectory(framework)
add_subdirectories("${PROJECT_SOURCE_DIR}/projects")