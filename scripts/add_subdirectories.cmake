
function(add_subdirectories)
    # Check for the proper number of arguments.
    if(NOT ${ARGC} GREATER 0)
        message(FATAL_ERROR "add_subdirectories called with incorrect number of arguments")
    endif()

    file(GLOB DIRECTORIES
         LIST_DIRECTORIES TRUE
         RELATIVE "${ARGV0}"
         "${ARGV0}/*")

    foreach (DIRECTORY ${DIRECTORIES})
        if (IS_DIRECTORY "${ARGV0}/${DIRECTORY}")
            add_subdirectory("${ARGV0}/${DIRECTORY}")
        endif ()
    endforeach ()
endfunction()
