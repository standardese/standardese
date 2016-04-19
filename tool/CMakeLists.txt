# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(header filesystem.hpp)
set(src main.cpp)

add_executable(standardese ${header} ${src})
comp_target_features(standardese PRIVATE CPP11)
target_link_libraries(standardese PUBLIC standardese_library)

find_package(Boost COMPONENTS program_options filesystem REQUIRED)
target_include_directories(standardese PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(standardese PUBLIC ${Boost_LIBRARIES})
