# Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
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
find_program(CLANG_BINARY "clang++" "/usr/bin" "/usr/local/bin")
if(NOT CLANG_BINARY)
    message(WARNING "unable to find clang binary, please set CLANG_BINARY yourself or pass --compilation.clang_binary")
    set(CLANG_BINARY "clang++")
endif()

set(CLANG_INCLUDE_PATHS "/usr/include/" "/usr/local/include")
set(CLANG_LIBRARY_PATHS "/usr/lib" "/usr/local/lib" "/usr/lib64")

# libclang headers and libs are installed in /usr/lib/llvm-<version> in Ubuntu.
file(GLOB LLVM_DIRS /usr/lib/llvm-*)
foreach(dir ${LLVM_DIRS})
    set(CLANG_INCLUDE_PATHS ${CLANG_INCLUDE_PATHS} ${dir}/include)
    set(CLANG_LIBRARY_PATHS ${CLANG_LIBRARY_PATHS} ${dir}/lib)
endforeach()

find_path(LIBCLANG_INCLUDE_DIR "clang-c/Index.h" ${CLANG_INCLUDE_PATHS})
if(NOT LIBCLANG_INCLUDE_DIR)
    message(FATAL_ERROR "unable to find libclang include directory, please set LIBCLANG_INCLUDE_DIR yourself")
endif()

find_library(LIBCLANG_LIBRARY "clang" ${CLANG_LIBRARY_PATHS})
if(NOT LIBCLANG_LIBRARY)
    message(FATAL_ERROR "unable to find libclang library, please set LIBCLANG_LIBRARY yourself")
endif()

if(NOT LIBCLANG_SYSTEM_INCLUDE_DIR)
    foreach(version "3.9.1" "3.9.0" "3.8.1" "3.8.0" "3.7.1")
        find_path(LIBCLANG_SYSTEM_INCLUDE_DIR_IMPL "clang/${version}/include" ${CLANG_LIBRARY_PATHS})
        set(libclang_version ${version})

        if(LIBCLANG_SYSTEM_INCLUDE_DIR_IMPL)
            break()
        endif()
    endforeach()
    if(NOT LIBCLANG_SYSTEM_INCLUDE_DIR_IMPL)
        message(FATAL_ERROR "${libclang_version} unable to find clang's system header files, please set LIBCLANG_SYSTEM_INCLUDE_DIR yourself")
    endif()

    message(STATUS "Libclang version: ${libclang_version}")
    set(LIBCLANG_SYSTEM_INCLUDE_DIR "${LIBCLANG_SYSTEM_INCLUDE_DIR_IMPL}/clang/${libclang_version}/include")
endif()

#
# add cppast
#
message(STATUS "Installing cppast via submodule")
execute_process(COMMAND git submodule update --init -- external/cppast
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(external/cppast)

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

#
# add cmark
#
find_library(CMARK_LIBRARY "cmark-gfm" "/usr/lib" "/usr/local/lib")
find_path(CMARK_INCLUDE_DIR "cmark_extension_api.h" "/usr/include" "/usr/local/include")

if((NOT CMARK_LIBRARY) OR (NOT CMARK_INCLUDE_DIR))
    message("Unable to find cmark, cloning...")
    execute_process(COMMAND git submodule update --init -- external/cmark
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # add and exclude targets
    add_subdirectory(external/cmark ${CMAKE_CURRENT_BINARY_DIR}/cmark)
    set_target_properties(api_test PROPERTIES EXCLUDE_FROM_ALL 1)

    # fixup target properties
    target_include_directories(libcmark-gfm_static PUBLIC
                               $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/cmark/src>
                               $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/cmark/src>)
    target_compile_definitions(libcmark-gfm_static PUBLIC CMARK_STATIC_DEFINE)

    # disable some warnings under MSVC, they're very noisy
    if(MSVC)
        target_compile_options(libcmark-gfm_static PRIVATE /wd4204 /wd4267 /wd4204 /wd4221 /wd4244 /wd4232)
    endif()
else()
    add_library(libcmark-gfm_static INTERFACE)
    target_include_directories(libcmark-gfm_static INTERFACE ${CMARK_INCLUDE_DIR})
    target_link_libraries(libcmark-gfm_static INTERFACE ${CMARK_LIBRARY})

    # install fake target
    install(TARGETS libcmark-gfm_static EXPORT standardese DESTINATION ${lib_dir})
endif()

#
# add tiny-process-library
#
if((NOT TINY_PROCESS_LIBRARY_INCLUDE_DIR) OR (NOT EXISTS TINY_PROCESS_LIBRARY_INCLUDE_DIR))
    message("Unable to find tiny-process-library, cloning...")
    execute_process(COMMAND git submodule update --init -- external/tiny-process-library
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set(TINY_PROCESS_LIBRARY_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/tiny-process-library
        CACHE PATH "tiny-process-library include directory")
    # treat as header only library and include source files
endif()
