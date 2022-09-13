
project(framework)

# Build framework as an object library.
add_library(${PROJECT_NAME} OBJECT
            "${CMAKE_SOURCE_DIR}/framework/src/core/entry_point.cpp"
            )

# External applications should only include 'framework/include/framework.hpp'.
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/framework/include")
