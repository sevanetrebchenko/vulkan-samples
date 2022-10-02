
project(framework)

# Build framework as an object library.
add_library(${PROJECT_NAME} OBJECT
            # core
            "${PROJECT_SOURCE_DIR}/src/core/entry_point.cpp"
            "${PROJECT_SOURCE_DIR}/src/core/window.cpp"
            "${PROJECT_SOURCE_DIR}/src/core/sample.cpp"
            "${PROJECT_SOURCE_DIR}/src/core/assert.cpp"

            # vulkan
            "${PROJECT_SOURCE_DIR}/src/vulkan/context.cpp"
            "${PROJECT_SOURCE_DIR}/src/vulkan/device.cpp"
            "${PROJECT_SOURCE_DIR}/src/vulkan/instance.cpp"
            "${PROJECT_SOURCE_DIR}/src/vulkan/types.cpp"
        )

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

# External applications should only include 'framework/include/framework.hpp'.
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
