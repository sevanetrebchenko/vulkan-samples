
# add_sample(<name> [ sample name ]
#            <source_files> [ ... ]
#            <include_directories> [ ... ]
#            <dependencies> [ ... ])
function(add_sample)
    # Helper macro for printing 'add_sample' usage.
    macro(error MESSAGE)
        message(FATAL_ERROR " [add_sample]: ${MESSAGE}: \n"
                " \t\t usage: add_sample(<name> [ sample name ] \n "
                "                        <source_files> [ ... ] \n "
                "                        <include_directories> [ ... ] (optional) \n "
                "                        <dependencies> [ ... ] (optional) )")
    endmacro()

    set(OPTIONS)
    set(SINGLE_VALUE_KEYWORDS NAME)
    set(MULTI_VALUE_KEYWORDS SOURCE_FILES INCLUDE_DIRECTORIES DEPENDENCIES)
    cmake_parse_arguments(ARG "${OPTIONS}" "${SINGLE_VALUE_KEYWORDS}" "${MULTI_VALUE_KEYWORDS}" ${ARGN})

    # Specify sample project name with the NAME keyword.
    if (NOT ARG_NAME)
        error("required <name> keyword is missing")
    endif ()

    project("${ARG_NAME}")
    message(STATUS "BUILDING SAMPLE: ${PROJECT_NAME}")

    list(APPEND CMAKE_MESSAGE_INDENT "       ")

    # Specify sample source files with the SOURCE_FILES keyword.
    if (NOT ARG_SOURCE_FILES)
        error("required <source_files> keyword is missing")
    endif ()

    if (SOURCE_FILES IN_LIST ARG_KEYWORDS_MISSING_VALUES)
        error("<source_files> keyword requires at least one valid argument")
    endif ()

    message("SOURCE FILES: ")
    list(APPEND CMAKE_MESSAGE_INDENT "    ")

    # Prepend each source file with the sample project root directory.
    set(SOURCE_FILES "")
    # list(REMOVE_DUPLICATES "${ARG_SOURCE_FILES}")
    foreach (FILE ${ARG_SOURCE_FILES})
        set(FILE "${PROJECT_SOURCE_DIR}/${FILE}")
        list(APPEND SOURCE_FILES ${FILE})

        string(REPLACE "${CMAKE_SOURCE_DIR}/" "${CMAKE_PROJECT_NAME}/" FILE ${FILE})
        message("- ${FILE}")
    endforeach ()
    add_executable(${PROJECT_NAME} "${SOURCE_FILES}")

    list(POP_BACK CMAKE_MESSAGE_INDENT)

    # Specify sample include directories with the INCLUDE_DIRECTORIES keyword.
    if (INCLUDE_DIRECTORIES IN_LIST ARG_KEYWORDS_MISSING_VALUES)
        error("<include_directories> keyword requires at least one valid argument")
    endif ()

    # Prepend each include directory with the sample project root directory.
    if (ARG_INCLUDE_DIRECTORIES)
        message("INCLUDE DIRECTORIES: ")
        list(APPEND CMAKE_MESSAGE_INDENT "    ")

        set(INCLUDE_DIRECTORIES "")
        foreach (DIRECTORY ${ARG_INCLUDE_DIRECTORIES})
            set(DIRECTORY "${PROJECT_SOURCE_DIR}/${DIRECTORY}")

            if (IS_DIRECTORY "${DIRECTORY}")
                list(APPEND INCLUDE_DIRECTORIES ${DIRECTORY})

                string(REPLACE "${CMAKE_SOURCE_DIR}/" "${CMAKE_PROJECT_NAME}/" DIRECTORY ${DIRECTORY})
                message("- ${DIRECTORY}")
            endif ()
        endforeach ()

        target_include_directories(${PROJECT_NAME} PRIVATE "${INCLUDE_DIRECTORIES}")

        list(POP_BACK CMAKE_MESSAGE_INDENT)
    endif ()

    # Specify sample project link dependencies with the DEPENDENCIES keyword.
    if (DEPENDENCIES IN_LIST ARG_KEYWORDS_MISSING_VALUES)
        error("<dependencies> keyword requires at least one valid argument")
    endif ()

    if (ARG_DEPENDENCIES)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${ARG_DEPENDENCIES})
    endif ()

    target_link_libraries(${PROJECT_NAME} PRIVATE framework) # Always link sample with the 'framework' object library.

    list(POP_BACK CMAKE_MESSAGE_INDENT)

endfunction()