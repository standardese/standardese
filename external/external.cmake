# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

#
# add compatibility
#
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

#
# add libclang
#
set(CLANG_INCLUDE_PATHS "/usr/include/" "/usr/local/include")
set(CLANG_LIBRARY_PATHS "/usr/lib" "/usr/local/lib")

# libclang headers and libs are installed in /usr/lib/llvm-<version> in Ubuntu.
file(GLOB LLVM_DIRS /usr/lib/llvm-*)
foreach(dir ${LLVM_DIRS})
    set(CLANG_INCLUDE_PATHS ${CLANG_INCLUDE_PATHS} ${dir}/include)
    set(CLANG_LIBRARY_PATHS ${CLANG_LIBRARY_PATHS} ${dir}/lib)
endforeach()

find_path(LIBCLANG_INCLUDE_DIR "clang-c/Index.h" ${CLANG_INCLUDE_PATHS})
if(NOT LIBCLANG_INCLUDE_DIR)
    message(FATAL_ERROR "unable to find libclang include directory, please set LIBCLANG_INCLUDE_DIR by yourself")
endif()

find_library(LIBCLANG_LIBRARY "clang" ${CLANG_LIBRARY_PATHS})
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

#
# add spdlog
#
if((NOT SPDLOG_INCLUDE_DIR) OR (NOT EXISTS ${SPDLOG_INCLUDE_DIR}))
    message("Unable to find spdlog, cloning...")
    execute_process(COMMAND git submodule update --init -- external/spdlog
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set(SPDLOG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/spdlog/include/
        CACHE PATH "spdlog include directory")
endif()
install(DIRECTORY ${SPDLOG_INCLUDE_DIR}/spdlog DESTINATION ${include_dest})

#
# add ThreadPool
#
if((NOT THREADPOOL_INCLUDE_DIR) OR (NOT EXISTS ${THREADPOOL_INCLUDE_DIR}))
    message("Unable to find ThreadPool, cloning...")
    execute_process(COMMAND git submodule update --init -- external/ThreadPool
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set(THREADPOOL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/ThreadPool
        CACHE PATH "ThreadPool include directory")
    # don't need to be installed
endif()
