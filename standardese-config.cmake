# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# EXTERNAL
# makes imported targets available
get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(${SELF_DIR}/${CMAKE_BUILD_TYPE}/standardese.cmake)
find_package(Threads REQUIRED)

# EXTERNAL
# saves the location of the standardese executable in STANDARDESE_TOOL
find_program(STANDARDESE_TOOL standardese)

# EXTERNAL
# generates documentation for a given target
# will create a custom target standardese_${target} that will run standardese with given options
# usage:
# standardese_generate(<target> [ALL] [CONFIG <config>]
#                      [INCLUDE_DIRECTORY <include_dir_a> [include_dir_b ...]]
#                      [MACRO_DEFINITION <def_a> [def_b ...]]
#                      [MACRO_UNDEFINITION <def_a> [def_b ...]]
#                      INPUT <input_a> [input_b ...])
# ALL - whether or not the custom target will run when building all
# CONFIG - same as --config for standardese
# INCLUDE_DIRECTORY - same as -I <arg> for standardese for each argument
# MACRO_DEFINITION - same as -D <arg> for standardese for each argument
# MACRO_UNDEFINITION - same as -U <arg> for standardese for each argument
# INPUT - the input files given to standardese
# all paths must be absolute (e.g. through CMAKE_CURRENT_SOURCE_DIR or similar)
# or relative to the working directory of standardese which is ${CMAKE_CURRENT_BINARY_DIR}/standardese_${target}
function(standardese_generate target)
    cmake_parse_arguments(STANDARDESE "ALL" # no arg
                                      "CONFIG" # single arg
                                      "INPUT;INCLUDE_DIRECTORY;MACRO_DEFINITION;MACRO_UNDEFINITION" # multiple arg
                          ${ARGN})

    if(STANDARDESE_CONFIG)
        list(APPEND options --config ${STANDARDESE_CONFIG})
    endif()

    if(STANDARDESE_INCLUDE_DIRECTORY)
        foreach(dir ${STANDARDESE_INCLUDE_DIRECTORY})
            list(APPEND options -I ${dir})
        endforeach()
    endif()

    if(STANDARDESE_MACRO_DEFINITION)
        foreach(def ${STANDARDESE_MACRO_DEFINITION})
            list(APPEND options -D ${def})
        endforeach()
    endif()

    if(STANDARDESE_MACRO_UNDEFINITION)
        foreach(def ${STANDARDESE_MACRO_UNDEFINITION})
            list(APPEND options -U ${def})
        endforeach()
    endif()

    foreach(input ${STANDARDESE_INPUT})
        list(APPEND options ${input})
    endforeach()

    if(STANDARDESE_ALL)
        set(all ALL)
    endif()

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/standardese_${target})
    add_custom_target(standardese_${target} ${all}
                      ${STANDARDESE_TOOL} ${options}
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/standardese_${target}
                      COMMENT "Generating documentation for target ${target}..."
                      VERBATIM)

endfunction()
