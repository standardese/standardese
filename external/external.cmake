# Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

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
