
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
add_subdirectory(glfw)

set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF)
set(TINYGLTF_INSTALL OFF)
add_subdirectory(gltf)

add_subdirectory(glm)

message(STATUS "Pulling shaderc dependencies")
find_package(Python3 COMPONENTS Interpreter REQUIRED)
execute_process(
    COMMAND ${Python3_EXECUTABLE} "${PROJECT_SOURCE_DIR}/lib/shaderc/utils/git-sync-deps"
    RESULT_VARIABLE RESULT
    OUTPUT_VARIABLE OUTPUT
    ERROR_VARIABLE ERROR
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
)
if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "Error: ${ERROR}")
endif()
set(SHADERC_SKIP_TESTS ON)
set(SHADERC_SKIP_EXAMPLES ON)
add_subdirectory(shaderc)

add_subdirectory(tinyobjloader)

add_subdirectory(stb)