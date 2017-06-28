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
    ../include/standardese/markup/link.hpp
    ../include/standardese/markup/list.hpp
    ../include/standardese/markup/paragraph.hpp
    ../include/standardese/markup/phrasing.hpp
    ../include/standardese/markup/quote.hpp
    ../include/standardese/markup/thematic_break.hpp)
set(header)
set(comment_src
    comment/cmark_ext_command.hpp
    comment/cmark_ext_command.cpp
    comment/config.cpp
    comment/doc_comment.cpp
    comment/parser.cpp)
set(markup_src
    markup/code_block.cpp
    markup/doc_section.cpp
    markup/document.cpp
    markup/documentation.cpp
    markup/entity_kind.cpp
    markup/generator.cpp
    markup/heading.cpp
    markup/html.cpp
    markup/link.cpp
    markup/list.cpp
    markup/paragraph.cpp
    markup/phrasing.cpp
    markup/quote.cpp
    markup/thematic_break.cpp
    markup/xml.cpp)
set(src)

add_library(standardese ${detail_header} ${comment_header} ${markup_header} ${header} ${comment_src} ${markup_src} ${src})
target_include_directories(standardese PUBLIC
                                $<BUILD_INTERFACE:${STANDARDESE_SOURCE_DIR}/include>
                                $<BUILD_INTERFACE:${SPDLOG_INCLUDE_DIR}>
                                $<BUILD_INTERFACE:${_standardese_comp_include}>
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

# link Boost
find_package(Boost COMPONENTS filesystem system thread date_time chrono REQUIRED)
target_include_directories(standardese PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(standardese PUBLIC ${Boost_LIBRARIES})

# install library: TODO
#install(TARGETS standardese EXPORT standardese DESTINATION "${lib_dest}")
#install(FILES ${header} DESTINATION "${include_dest}/standardese")
#install(FILES ${detail_header} DESTINATION "${include_dest}/standardese/detail")
