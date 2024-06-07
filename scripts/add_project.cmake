
# Usage:
# add_project(<name> [ project name ]
#             <source_files> [ ... ]
#             <include_directories> [ ... ]
#             <dependencies> [ ... ])
function(add_project)
    # Helper macro for printing 'add_project' usage
    macro(error MESSAGE)
        message(FATAL_ERROR " [add_project]: ${MESSAGE}: \n"
                " \t\t usage: add_project(<name> [ project name ] \n "
                "                         <source_files> [ ... ] \n "
                "                         <include_directories> [ ... ] (optional) \n "
                "                         <dependencies> [ ... ] (optional) )")
    endmacro()

    set(OPTIONS)
    set(SINGLE_VALUE_KEYWORDS NAME)
    set(MULTI_VALUE_KEYWORDS SOURCE_FILES INCLUDE_DIRECTORIES DEPENDENCIES)
    cmake_parse_arguments(ARG "${OPTIONS}" "${SINGLE_VALUE_KEYWORDS}" "${MULTI_VALUE_KEYWORDS}" ${ARGN})

    # SECTION: Project name -------------------------------------------------------------------------------------------

    # Specify project name with the NAME keyword
    if (NOT ARG_NAME)
        error("required <name> keyword is missing")
    endif ()
    project("${ARG_NAME}")

    # SECTION: Project source -----------------------------------------------------------------------------------------

    # Specify source files with the SOURCE_FILES keyword
    if (NOT ARG_SOURCE_FILES)
        error("required <source_files> keyword is missing")
    endif ()

    if (SOURCE_FILES IN_LIST ARG_KEYWORDS_MISSING_VALUES)
        error("<source_files> keyword requires at least one valid argument")
    endif ()

    # Prepend each source file with the sample project root directory
    set(SOURCE_FILES "")
    # list(REMOVE_DUPLICATES "${ARG_SOURCE_FILES}")
    foreach (FILE ${ARG_SOURCE_FILES})
        list(APPEND SOURCE_FILES "${PROJECT_SOURCE_DIR}/${FILE}")
    endforeach ()

    add_executable(${PROJECT_NAME} "${SOURCE_FILES}")

    # SECTION: Project include directories ----------------------------------------------------------------------------

    # Specify include directories with the INCLUDE_DIRECTORIES keyword
    if (INCLUDE_DIRECTORIES IN_LIST ARG_KEYWORDS_MISSING_VALUES)
        error("<include_directories> keyword requires at least one valid argument")
    endif ()

    # Prepend each include directory with the sample project root directory
    if (ARG_INCLUDE_DIRECTORIES)
        set(INCLUDE_DIRECTORIES "")
        foreach (DIRECTORY ${ARG_INCLUDE_DIRECTORIES})
            if (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/${DIRECTORY}")
                list(APPEND INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/${DIRECTORY}")
            endif ()
        endforeach ()

        target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRECTORIES})
    endif ()

    # SECTION: Project dependencies -----------------------------------------------------------------------------------

    # Specify project link dependencies with the DEPENDENCIES keyword
    if (DEPENDENCIES IN_LIST ARG_KEYWORDS_MISSING_VALUES)
        error("<dependencies> keyword requires at least one valid argument")
    endif ()

    if (ARG_DEPENDENCIES)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${ARG_DEPENDENCIES})
    endif ()

    target_link_libraries(${PROJECT_NAME} PRIVATE framework) # Always link project with the 'framework' object library

endfunction()