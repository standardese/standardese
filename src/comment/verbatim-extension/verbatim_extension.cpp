// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "verbatim_extension.hpp"

#include <cmark-gfm-extension_api.h>

namespace standardese::comment::verbatim_extension
{

verbatim_extension::verbatim_extension(cmark_syntax_extension* extension)
{
    cmark_syntax_extension_set_get_type_string_func(extension, verbatim_extension::cmark_get_type_string);

    cmark_syntax_extension_set_special_inline_chars(
        extension, cmark_llist_append(cmark_get_default_mem_allocator(), nullptr, reinterpret_cast<void*>('\\')));

    cmark_syntax_extension_set_match_inline_func(extension, verbatim_extension::cmark_match_inline);
}

verbatim_extension& verbatim_extension::create(cmark_parser* parser)
{
    auto* cmark_extension = cmark_syntax_extension_new("standardese_verbatim");
    auto* extension = new verbatim_extension(cmark_extension);
    cmark_syntax_extension_set_private(
        cmark_extension,
        extension,
        [](cmark_mem*, void* data) {
          delete static_cast<verbatim_extension*>(data);
        }
    );
    cmark_parser_attach_syntax_extension(parser, cmark_extension);
    return *extension;
}

const char* verbatim_extension::cmark_get_type_string(cmark_syntax_extension* extension, cmark_node* node){
  return cmark_node_get_type(node) == node_type() ? "verbatim" : "<unknown>";
}

cmark_node* verbatim_extension::cmark_match_inline(cmark_syntax_extension* extension, cmark_parser*, cmark_node* parent, unsigned char, cmark_inline_parser* parser)
{
    // Return whether `command` starts here.
    const auto advance_if = [&](const std::string& command) {
        const auto offset = cmark_inline_parser_get_offset(parser);

        while (true)
        {
            const size_t i = cmark_inline_parser_get_offset(parser) - offset;

            if (cmark_inline_parser_is_eof(parser) || cmark_inline_parser_peek_char(parser) != command[i])
            {
                cmark_inline_parser_set_offset(parser, offset);
                return false;
            }

            cmark_inline_parser_advance_offset(parser);

            if (i == command.size() - 1)
                return true;
        }
    };

    // Check whether there is a \verbatim command at this offset.
    if (!advance_if("verbatim "))
        return nullptr;

    // Remove the `\` in `\verbatim` from the previous node.
    cmark_node_unput(parent, 1);

    // Read the content of the verbatim command, i.e., until the end of the line or an explicit \end command.
    std::string content;

    while (true)
    {
        if (cmark_inline_parser_is_eof(parser) || cmark_inline_parser_peek_char(parser) == '\n' || advance_if(R"(\end)"))
            break;

        content += cmark_inline_parser_peek_char(parser);
        cmark_inline_parser_advance_offset(parser);
    }

    cmark_node* node = cmark_node_new(node_type());
    cmark_node_set_string_content(node, content.c_str());
    cmark_node_set_syntax_extension(node, extension);

    return node;
}

cmark_node_type verbatim_extension::node_type() {
    static const auto type = cmark_syntax_extension_add_node(1);
    return type;
}

verbatim_extension::~verbatim_extension() {}

}
