
project(framework)

# Build as a static library
add_library(${PROJECT_NAME} STATIC
#    "${PROJECT_SOURCE_DIR}/src/sample.cpp"
#    "${PROJECT_SOURCE_DIR}/src/helpers.cpp"
#    "${PROJECT_SOURCE_DIR}/src/camera.cpp"
#    "${PROJECT_SOURCE_DIR}/src/vulkan_initializers.cpp"
#    "${PROJECT_SOURCE_DIR}/src/loaders/obj.cpp"
#    "${PROJECT_SOURCE_DIR}/src/loaders/gltf.cpp"
#    "${PROJECT_SOURCE_DIR}/src/transform.cpp"
#    "${PROJECT_SOURCE_DIR}/src/renderpass.cpp"

#    "${PROJECT_SOURCE_DIR}/src/context.cpp"
#    "${PROJECT_SOURCE_DIR}/src/window.cpp"
#    "${PROJECT_SOURCE_DIR}/src/shader.cpp"
#
#    "${PROJECT_SOURCE_DIR}/src/vks/detail/shader_compiler.cpp"
#
#    "${PROJECT_SOURCE_DIR}/src/vulkan/device.cpp"
#    "${PROJECT_SOURCE_DIR}/src/vulkan/shader.cpp"
#    "${PROJECT_SOURCE_DIR}/src/vulkan/pipeline.cpp"
#    "${PROJECT_SOURCE_DIR}/src/vulkan/utility.cpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/device.cpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/queue.cpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/render_context.cpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/render_graph.cpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/shader.cpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/swapchain.cpp"
    "${PROJECT_SOURCE_DIR}/src/sample.cpp"
    "${PROJECT_SOURCE_DIR}/src/window.cpp"
)

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE Vulkan::Vulkan)

add_subdirectory(lib)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)
target_link_libraries(${PROJECT_NAME} PUBLIC glm)
target_link_libraries(${PROJECT_NAME} PUBLIC shaderc)
target_link_libraries(${PROJECT_NAME} PUBLIC stb)
target_link_libraries(${PROJECT_NAME} PUBLIC assimp)
target_link_libraries(${PROJECT_NAME} PUBLIC spirv-reflect-static)
target_link_libraries(${PROJECT_NAME} PUBLIC utils)

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
# target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src") # Private engine headers are mixed in with project source