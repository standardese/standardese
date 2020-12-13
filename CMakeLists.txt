# Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.1)
project(STANDARDESE VERSION 0.4.1)

option(STANDARDESE_BUILD_TOOL "Build the standardese binary" ON)
option(STANDARDESE_BUILD_TEST "Build the standardese test suite" ON)
option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so/.dylib) instead of static ones (.lib/.a)" ON)

set(lib_dest "lib/standardese")
set(include_dest "include")
set(tool_dest "bin")

# add external libraries
include(external/external.cmake)

if (MSVC)
    add_compile_options(
        # Disable warnings
        -D_CRT_SECURE_NO_WARNINGS
        -D_SCL_SECURE_NO_WARNINGS
        /wd4503 # decorated name length exceeded, name was truncated
        /MP # Multi-processor compilation
        )
endif()

# subdirectories
add_subdirectory(src)
if(STANDARDESE_BUILD_TOOL)
    add_subdirectory(tool)
endif()
if (STANDARDESE_BUILD_TEST)
    add_subdirectory(test)
endif()

# install configuration
#install(EXPORT standardese DESTINATION "${lib_dest}")
#install(FILES standardese-config.cmake LICENSE DESTINATION "${lib_dest}")
