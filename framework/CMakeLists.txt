
project(framework)

# Build as a static library
add_library(${PROJECT_NAME} STATIC
    "${PROJECT_SOURCE_DIR}/src/sample.cpp"
    "${PROJECT_SOURCE_DIR}/src/helpers.cpp"
)

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE Vulkan::Vulkan)

add_subdirectory(lib)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)
target_link_libraries(${PROJECT_NAME} PUBLIC glm)
target_link_libraries(${PROJECT_NAME} PUBLIC shaderc)

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
# target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src") # Private engine headers are mixed in with project source