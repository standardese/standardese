# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.1)
project(STANDARDESE VERSION 0.1)

option(STANDARDESE_BUILD_TOOL "whether or not to build the tool" ON)
option(STANDARDESE_BUILD_TEST "whether or not to build the test" ON)

set(lib_dest "lib/standardese")
set(include_dest "include")
set(tool_dest "bin")

# add compatibility
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/comp_base.cmake)
    file(DOWNLOAD https://raw.githubusercontent.com/foonathan/compatibility/master/comp_base.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/comp_base.cmake)
endif()

include(${CMAKE_CURRENT_BINARY_DIR}/comp_base.cmake)
add_library(_standardese_comp_runner INTERFACE)
set(_standardese_comp_include ${CMAKE_CURRENT_BINARY_DIR}/comp.generated/)
comp_target_features(_standardese_comp_runner INTERFACE
                     cpp11_lang/noexcept
                     PREFIX "STANDARDESE_" NAMESPACE "standardese_comp"
                     INCLUDE_PATH ${_standardese_comp_include}
                     NOFLAGS)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/comp.generated/standardese DESTINATION "${include_dest}/comp")

# add libclang
find_path(LIBCLANG_INCLUDE_DIR "clang-c/Index.h" "/usr/include/" "/usr/local/include")
if(NOT LIBCLANG_INCLUDE_DIR)
    message(FATAL_ERROR "unable to find libclang include directory, please set LIBCLANG_INCLUDE_DIR by yourself")
endif()

find_library(LIBCLANG_LIBRARY "clang" "/usr/lib" "/usr/local/lib")
if(NOT LIBCLANG_LIBRARY)
    message(FATAL_ERROR "unable to find libclang library, please set LIBCLANG_LIBRARY by yourself")
endif()

if(NOT LIBCLANG_SYSTEM_INCLUDE_DIR)
    find_path(LIBCLANG_SYSTEM_INCLUDE_DIR_IMPL "clang/3.8.0/include" "/usr/lib/" "/usr/local/lib")
    set(libclang_version 3.8.0)

    if(NOT LIBCLANG_SYSTEM_INCLUDE_DIR_IMPL)
        find_path(LIBCLANG_SYSTEM_INCLUDE_DIR_IMPL "clang/3.7.1/include" "/usr/lib/" "/usr/local/lib")
        set(libclang_version 3.7.1)
    endif()

    if(NOT LIBCLANG_SYSTEM_INCLUDE_DIR_IMPL)
        message(FATAL_ERROR "${libclang_version} unable to find clang's system header files, please set LIBCLANG_SYSTEM_INCLUDE_DIR by yourself")
    endif()

    message(STATUS "Libclang version: ${libclang_version}")
    set(LIBCLANG_SYSTEM_INCLUDE_DIR "${LIBCLANG_SYSTEM_INCLUDE_DIR_IMPL}/clang/${libclang_version}/include")
endif()

# add spdlog
if((NOT SPDLOG_INCLUDE_DIR) OR (NOT EXISTS ${SPDLOG_INCLUDE_DIR}))
    message("Unable to find spdlog, downloading...")
    file(DOWNLOAD https://github.com/gabime/spdlog/archive/master.zip
         ${CMAKE_CURRENT_BINARY_DIR}/spdlog.zip)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf spdlog.zip
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set(SPDLOG_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/spdlog-master/include/
        CACHE PATH "spdlog include directory")
endif()
install(DIRECTORY ${SPDLOG_INCLUDE_DIR}/spdlog DESTINATION ${include_dest})

if (MSVC)
    add_compile_options(
        -D_CRT_SECURE_NO_WARNINGS
        -D_SCL_SECURE_NO_WARNINGS
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
install(EXPORT standardese DESTINATION "${lib_dest}")
install(FILES standardese-config.cmake spdlog-license.md LICENSE DESTINATION "${lib_dest}")
