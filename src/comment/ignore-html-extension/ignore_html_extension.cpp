// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
//                    2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "ignore_html_extension.hpp"
#include <cmark-gfm.h>
#include <cassert>

namespace standardese::comment::ignore_html_extension
{

ignore_html_extension::ignore_html_extension(cmark_syntax_extension* extension)
{
    cmark_syntax_extension_set_postprocess_func(extension, ignore_html_extension::cmark_postprocess);
}

ignore_html_extension& ignore_html_extension::create(cmark_parser* parser)
{
    auto* cmark_extension = cmark_syntax_extension_new("standardese_ignore_html");
    auto* extension = new ignore_html_extension(cmark_extension);
    cmark_syntax_extension_set_private(
        cmark_extension,
        extension,
        [](cmark_mem*, void* data) {
          delete static_cast<ignore_html_extension*>(data);
        }
    );
    cmark_parser_attach_syntax_extension(parser, cmark_extension);
    return *extension;
}

cmark_node* ignore_html_extension::cmark_postprocess(cmark_syntax_extension* extension, cmark_parser*, cmark_node* root)
{
    auto iter = cmark_iter_new(root);

    // Iterate over the tree of CommonMark nodes and postprocess each node.
    while (true)
    {
        switch (cmark_iter_next(iter))
        {
            case CMARK_EVENT_DONE:
                return nullptr;
            case CMARK_EVENT_EXIT:
            case CMARK_EVENT_NONE:
                break;
            case CMARK_EVENT_ENTER:
                cmark_iter_reset(iter, postprocess(cmark_iter_get_node(iter)), CMARK_EVENT_ENTER);
                break;
        }
    }

    return nullptr;
}

cmark_node* ignore_html_extension::postprocess(cmark_node* node)
{
    switch (cmark_node_get_type(node))
    {
        case CMARK_NODE_HTML_BLOCK:
        {
            // This is an HTML block, i.e., a "paragraph" that starts with a
            // `<`. This probably should not happen in C++ documentation but we
            // turn it back into text for consistency.
            cmark_node* paragraph = cmark_node_new(CMARK_NODE_PARAGRAPH);
            cmark_node* text = cmark_node_new(CMARK_NODE_TEXT);

            cmark_node_set_literal(text, cmark_node_get_literal(node));

            cmark_node_append_child(paragraph, text);
            cmark_node_replace(node, paragraph);
            cmark_node_free(node);

            return paragraph;
        }
        case CMARK_NODE_HTML_INLINE:
        {
            // This is (misinterpreted) inline HTML, e.g., the `<T>` in
            // `vector<T>`. We turn it back into a text node and merge it with
            // any surrounding text nodes.
            cmark_node* text = cmark_node_new(CMARK_NODE_TEXT);
            cmark_node_set_literal(text, cmark_node_get_literal(node));

            cmark_node_replace(node, text);
            cmark_node_free(node);

            // Merge with the previous node.
            text = merge(cmark_node_previous(text), text);

            // Merge with the following node.
            text = merge(text, cmark_node_next(text));

            return text;
        }
        default:
            return node;
    }
}

cmark_node* ignore_html_extension::merge(cmark_node* lhs, cmark_node* rhs)
{
    if (lhs == nullptr || cmark_node_get_type(lhs) != CMARK_NODE_TEXT)
    {
        assert(rhs != nullptr && cmark_node_get_type(rhs) == CMARK_NODE_TEXT);

        return rhs;
    }

    if (rhs == nullptr || cmark_node_get_type(rhs) != CMARK_NODE_TEXT)
    {
        return lhs;
    }

    const auto merged = cmark_node_get_literal(lhs) + std::string(cmark_node_get_literal(rhs));
    cmark_node_set_literal(lhs, merged.c_str());
    cmark_node_free(rhs);

    return lhs;
}

ignore_html_extension::~ignore_html_extension() {}

}
