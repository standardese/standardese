// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_CMARK_EXT_HPP_INCLUDED
#define STANDARDESE_COMMENT_CMARK_EXT_HPP_INCLUDED

#include <cmark-gfm.h>

#include <standardese/comment/config.hpp>

extern "C"
{
    typedef struct cmark_syntax_extension cmark_syntax_extension;
}

namespace standardese
{
namespace comment
{
    namespace detail
    {
        //=== command extension ===//
        cmark_syntax_extension* create_command_extension(config& c);

        cmark_node_type node_command();

        // returns the command type
        command_type get_command_type(cmark_node* node);

        // returns the arguments of the commands, unparsed
        const char* get_command_arguments(cmark_node* node);

        cmark_node_type node_section();

        // returns the section type
        section_type get_section_type(cmark_node* node);

        // returns the key if it is a key-value section
        // or nullptr, if it isn't
        const char* get_section_key(cmark_node* node);

        cmark_node_type node_inline();

        // returns the inline type
        inline_type get_inline_type(cmark_node* node);

        // returns the inline entity name
        const char* get_inline_entity(cmark_node* node);

        //=== verbatim extension ===//
        cmark_node_type node_verbatim();

        const char* get_verbatim_content(cmark_node* node);

        cmark_syntax_extension* create_verbatim_extension(config& c);

        //=== no HTML extension ===//
        cmark_syntax_extension* create_no_html_extension();
    } // namespace detail
} // namespace comment
} // namespace standardese

#endif // STANDARDESE_COMMENT_CMARK_EXT_HPP_INCLUDED
