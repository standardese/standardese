# Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(tests
    comment/parser.cpp
    markup/code_block.cpp
    markup/document.cpp
    markup/documentation.cpp
    markup/heading.cpp
    markup/index.cpp
    markup/link.cpp
    markup/list.cpp
    markup/paragraph.cpp
    markup/phrasing.cpp
    markup/quote.cpp
    markup/thematic_break.cpp
    comment.cpp
    doc_entity.cpp
    documentation.cpp
    index.cpp
    linker.cpp
    synopsis.cpp)

add_executable(standardese_test test.cpp test_logger.hpp test_parser.hpp ${tests})
target_include_directories(standardese_test PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(standardese_test PUBLIC standardese)
set_target_properties(standardese_test PROPERTIES CXX_STANDARD 17)

enable_testing()
add_test(NAME test COMMAND standardese_test)
