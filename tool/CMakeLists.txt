# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(header filesystem.hpp thread_pool.hpp)
set(src main.cpp)

add_executable(standardese_tool ${header} ${src})
comp_target_features(standardese_tool PRIVATE CPP11)
target_link_libraries(standardese_tool PUBLIC standardese)
target_include_directories(standardese_tool PUBLIC $<BUILD_INTERFACE:${THREADPOOL_INCLUDE_DIR}>)
set_target_properties(standardese_tool PROPERTIES OUTPUT_NAME standardese)

# link Boost

# Force linking to static libraries. Linking Boost.ProgramOptions dynamic library causes link errors
# See for example http://stackoverflow.com/questions/11235927/link-to-static-boost-lib-with-cmake-and-vs2010-without-automatic-linking.
set(Boost_USE_STATIC_LIBS ON)

find_package(Boost COMPONENTS program_options REQUIRED)
target_include_directories(standardese_tool PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(standardese_tool PUBLIC ${Boost_LIBRARIES})

# install tool
install(TARGETS standardese_tool EXPORT standardese DESTINATION "${tool_dest}")
