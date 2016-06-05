# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(header filesystem.hpp)
set(src main.cpp)

add_executable(standardese_tool ${header} ${src})
comp_target_features(standardese_tool PRIVATE CPP11)
target_link_libraries(standardese_tool PUBLIC standardese_tool_library)
set_target_properties(standardese_tool PROPERTIES OUTPUT_NAME standardese)

# link Boost
find_package(Boost COMPONENTS program_options filesystem REQUIRED)
target_include_directories(standardese_tool PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(standardese_tool PUBLIC ${Boost_LIBRARIES})

# install tool
install(TARGETS standardese_tool DESTINATION "bin")
