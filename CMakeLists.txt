# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.1)
project(STANDARDESE VERSION 0.0)

option(STANDARDESE_BUILD_TOOL "whether or not to build the tool" ON)
option(STANDARDESE_BUILD_TEST "whether or not to build the test" ON)

# add compatibility
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/comp_base.cmake)
    file(DOWNLOAD https://raw.githubusercontent.com/foonathan/compatibility/master/comp_base.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/comp_base.cmake)
endif()
include(${CMAKE_CURRENT_BINARY_DIR}/comp_base.cmake)

# add libclang
find_path(LIBCLANG_INCLUDE_DIR "clang-c/Index.h" "/usr/include/" "/usr/local/include")
if(NOT LIBCLANG_INCLUDE_DIR)
    message(FATAL_ERROR "unable to find libclang include directory, please set LIBCLANG_INCLUDE_DIR by yourself")
endif()

find_library(LIBCLANG_LIBRARY "clang" "/usr/lib" "/usr/local/lib")
if(NOT LIBCLANG_LIBRARY)
    message(FATAL_ERROR "unable to find libclang library, please set LIBCLANG_LIBRARY by yourself")
endif()

find_path(LIBCLANG_SYSTEM_INCLUDE_DIR "clang/3.7.1/include" "/usr/lib/" "/usr/local/lib")
if(NOT LIBCLANG_SYSTEM_INCLUDE_DIR)
    message(FATAL_ERROR "unable to find clang's system header files, please set LIBCLANG_SYSTEM_INCLUDE_DIR by yourself")
endif()

# subdirectories
add_subdirectory(src)
if(STANDARDESE_BUILD_TOOL)
    add_subdirectory(tool)
endif()
if (STANDARDESE_BUILD_TEST)
    add_subdirectory(test)
endif()
