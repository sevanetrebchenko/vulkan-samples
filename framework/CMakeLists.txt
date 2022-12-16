
project(framework)

# Build framework as an object library.
add_library(${PROJECT_NAME} OBJECT
    # public
    "${PROJECT_SOURCE_DIR}/public/src/framework.cpp"

    # graphics
    "${PROJECT_SOURCE_DIR}/public/src/graphics/context.cpp"

    # private
    # graphics
    # vulkan
    "${PROJECT_SOURCE_DIR}/private/src/graphics/vulkan/context.cpp"
    "${PROJECT_SOURCE_DIR}/private/src/graphics/vulkan/window.cpp"
    "${PROJECT_SOURCE_DIR}/private/src/graphics/vulkan/device.cpp"
)

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE Vulkan::Vulkan)

# Dynamically link against Vulkan.
target_compile_definitions(${PROJECT_NAME} PRIVATE VK_NO_PROTOTYPES)

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/public/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/private/include")
