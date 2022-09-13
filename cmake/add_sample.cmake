
# TODO:
# add sample(name
#            SOURCES ...
#            INCLUDE_DIRECTORIES ...
#            DEPENDENCIES ... )

function(add_sample)
    message(STATUS "Building sample: ${PROJECT_NAME}")

    # Framework source files are shared across all projects.
    add_executable(${PROJECT_NAME} ${SAMPLE_SOURCE_FILES})
    target_include_directories("${PROJECT_NAME}" PRIVATE "${SAMPLE_INCLUDE_DIRECTORIES}")

    target_link_libraries(${PROJECT_NAME} framework)
endfunction()