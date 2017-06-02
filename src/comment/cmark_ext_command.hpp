// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_CMARK_EXT_COMMAND_HPP_INCLUDED
#define STANDARDESE_COMMENT_CMARK_EXT_COMMAND_HPP_INCLUDED

#include <cmark.h>

#include <standardese/comment/config.hpp>

extern "C" {
typedef struct cmark_syntax_extension cmark_syntax_extension;
}

namespace standardese
{
    namespace comment
    {
        namespace detail
        {
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
        } // namespace detail
    }
} // namespace standardese::comment

#endif // STANDARDESE_COMMENT_CMARK_EXT_SECTION_HPP_INCLUDED
