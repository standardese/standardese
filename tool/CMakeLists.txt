# Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(header filesystem.hpp generator.hpp thread_pool.hpp)
set(src generator.cpp main.cpp)

add_executable(standardese_tool ${header} ${src})
target_link_libraries(standardese_tool PUBLIC standardese)
target_include_directories(standardese_tool PUBLIC $<BUILD_INTERFACE:${THREADPOOL_INCLUDE_DIR}>)
set_target_properties(standardese_tool PROPERTIES OUTPUT_NAME standardese CXX_STANDARD 11)

# link Boost
find_package(Boost COMPONENTS program_options filesystem system REQUIRED)
target_include_directories(standardese_tool PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(standardese_tool PUBLIC ${Boost_LIBRARIES})

# install tool
#install(TARGETS standardese_tool EXPORT standardese DESTINATION "${tool_dest}")
