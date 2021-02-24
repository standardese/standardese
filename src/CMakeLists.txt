# Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(comment_header
    ../include/standardese/comment/commands.hpp
    ../include/standardese/comment/config.hpp
    ../include/standardese/comment/doc_comment.hpp
    ../include/standardese/comment/matching_entity.hpp
    ../include/standardese/comment/metadata.hpp
    ../include/standardese/comment/parser.hpp)
set(markup_header
    ../include/standardese/markup/block.hpp
    ../include/standardese/markup/code_block.hpp
    ../include/standardese/markup/doc_section.hpp
    ../include/standardese/markup/document.hpp
    ../include/standardese/markup/documentation.hpp
    ../include/standardese/markup/entity.hpp
    ../include/standardese/markup/entity_kind.hpp
    ../include/standardese/markup/generator.hpp
    ../include/standardese/markup/heading.hpp
    ../include/standardese/markup/index.hpp
    ../include/standardese/markup/link.hpp
    ../include/standardese/markup/list.hpp
    ../include/standardese/markup/paragraph.hpp
    ../include/standardese/markup/phrasing.hpp
    ../include/standardese/markup/quote.hpp
    ../include/standardese/markup/thematic_break.hpp
    ../include/standardese/markup/visitor.hpp)
set(header
    ../include/standardese/comment.hpp
    ../include/standardese/doc_entity.hpp
    ../include/standardese/index.hpp
    ../include/standardese/linker.hpp
    ../include/standardese/logger.hpp)

set(comment_src
    comment/command-extension/command_extension.hpp
    comment/command-extension/command_extension.cpp
    comment/verbatim-extension/verbatim_extension.hpp
    comment/verbatim-extension/verbatim_extension.cpp
    comment/ignore-html-extension/ignore_html_extension.hpp
    comment/ignore-html-extension/ignore_html_extension.cpp
    comment/command-extension/user_data.hpp
    comment/command-extension/user_data.cpp
    comment/cmark-extension/cmark_extension.hpp
    comment/cmark-extension/cmark_extension.cpp
    comment/config.cpp
    comment/doc_comment.cpp
    comment/parser.cpp)
set(markup_src
    markup/block.cpp
    markup/code_block.cpp
    markup/doc_section.cpp
    markup/document.cpp
    markup/documentation.cpp
    markup/entity_kind.cpp
    markup/generator.cpp
    markup/heading.cpp
    markup/html.cpp
    markup/index.cpp
    markup/link.cpp
    markup/list.cpp
    markup/markdown.cpp
    markup/paragraph.cpp
    markup/phrasing.cpp
    markup/quote.cpp
    markup/thematic_break.cpp
    markup/visitor.cpp
    markup/xml.cpp)
set(src
    entity_visitor.hpp
    get_special_entity.hpp
    comment.cpp
    doc_entity.cpp
    index.cpp
    linker.cpp
    util/enum_values.hpp)

add_library(standardese ${detail_header} ${comment_header} ${markup_header} ${header} ${comment_src} ${markup_src} ${src})
set_target_properties(standardese PROPERTIES CXX_STANDARD 17)
target_include_directories(standardese PUBLIC
                                $<BUILD_INTERFACE:${STANDARDESE_SOURCE_DIR}/include>
                                $<INSTALL_INTERFACE:${include_dest}>
                                $<INSTALL_INTERFACE:${include_dest}/comp>)
target_compile_definitions(standardese PUBLIC
                                STANDARDESE=1
                                STANDARDESE_VERSION_MAJOR=${STANDARDESE_VERSION_MAJOR}
                                STANDARDESE_VERSION_MINOR=${STANDARDESE_VERSION_MINOR})

# add threading support
find_package(Threads REQUIRED)
target_link_libraries(standardese PUBLIC Threads::Threads)

# link cppast
target_link_libraries(standardese PUBLIC cppast)

# link cmark
target_link_libraries(standardese PUBLIC libcmark-gfm_static)

# install library: TODO
#install(TARGETS standardese EXPORT standardese DESTINATION "${lib_dest}")
#install(FILES ${header} DESTINATION "${include_dest}/standardese")
#install(FILES ${detail_header} DESTINATION "${include_dest}/standardese/detail")
