
cmake_minimum_required(VERSION 3.3)
project(vulkan-samples LANGUAGES CXX)

add_subdirectory(framework)
set(CMAKE_CXX_STANDARD 20)

# CMake helper modules.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(add_subdirectories)
include(add_sample)

# Call 'add_subdirectory' for each directory under vulkan-samples/samples.
add_subdirectories("${CMAKE_SOURCE_DIR}/samples")
