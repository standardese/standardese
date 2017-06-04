# Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# add catch
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)
    file(DOWNLOAD https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp
            ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)
endif()

set(tests
    comment/parser.cpp
    markup/code_block.cpp
    markup/document.cpp
    markup/documentation.cpp
    markup/heading.cpp
    markup/link.cpp
    markup/list.cpp
    markup/paragraph.cpp
    markup/phrasing.cpp
    markup/quote.cpp
    markup/thematic_break.cpp
    comment.cpp
    cpp_entity.cpp
    cpp_entity_blacklist.cpp
    cpp_function.cpp
    cpp_namespace.cpp
    cpp_template.cpp
    cpp_type.cpp
    cpp_variable.cpp
    output.cpp
    preprocessor.cpp
    template.cpp)

add_executable(standardese_test test.cpp test_parser.hpp ${tests})
target_include_directories(standardese_test PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(standardese_test PUBLIC standardese)
comp_target_features(standardese_test PUBLIC CPP11)

enable_testing()
add_test(NAME test COMMAND standardese_test)
