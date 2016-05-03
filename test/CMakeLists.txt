# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# add catch
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)
    file(DOWNLOAD https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp
            ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)
endif()

set(tests
        comment.cpp
        cpp_entity.cpp
        cpp_function.cpp
        cpp_namespace.cpp
        cpp_preprocessor.cpp
        cpp_template.cpp
        cpp_type.cpp
        cpp_variable.cpp)

add_executable(standardese_test test.cpp test_parser.hpp ${tests})
target_include_directories(standardese_test PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(standardese_test PUBLIC standardese_library)
comp_target_features(standardese_test PUBLIC CPP11)

enable_testing()
add_test(NAME test COMMAND standardese_test)
