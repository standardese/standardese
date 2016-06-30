// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment.hpp>

#include <cmark.h>
#include <stack>

#include <standardese/detail/wrapper.hpp>
#include <standardese/error.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

md_document::~md_document()
{
    cmark_node_free(get_node());
}

namespace
{
    struct parser_deleter
    {
        void operator()(cmark_parser* parser) const STANDARDESE_NOEXCEPT
        {
            cmark_parser_free(parser);
        }
    };

    using md_parser = detail::wrapper<cmark_parser*, parser_deleter>;

    void skip_comment(const char*& ptr)
    {
        while (std::isspace(*ptr))
            ++ptr;

        while (*ptr == '/')
            ++ptr;
    }

    md_ptr<md_document> parse_document(const cpp_raw_comment& raw_comment)
    {
        md_parser parser(cmark_parser_new(CMARK_OPT_NORMALIZE));

        auto        cur_start  = raw_comment.c_str();
        std::size_t cur_length = 0u;

        // skip leading comment
        skip_comment(cur_start);
        for (auto cur = cur_start; *cur;)
        {
            if (*cur == '\n')
            {
                // finished with one line
                // +1 to include newline
                cmark_parser_feed(parser.get(), cur_start, cur_length + 1);
                skip_comment(cur);
                cur_start  = cur;
                cur_length = 0u;
            }
            else
            {
                // part of the same line
                cur++;
                cur_length++;
            }
        }

        // final line
        cmark_parser_feed(parser.get(), cur_start, cur_length);

        auto doc_node = cmark_parser_finish(parser.get());
        return detail::make_md_ptr<md_document>(doc_node);
    }

    struct iter_deleter
    {
        void operator()(cmark_iter* iter) const STANDARDESE_NOEXCEPT
        {
            cmark_iter_free(iter);
        }
    };

    using md_iter = detail::wrapper<cmark_iter*, iter_deleter>;
}

comment comment::parse(const parser& p, const cpp_name& name, const cpp_raw_comment& raw_comment)
{
    auto document = parse_document(raw_comment);

    std::stack<md_container*> containers;
    containers.push(document.get());

    comment result(std::move(document));

    auto    ev = CMARK_EVENT_NONE;
    md_iter iter(cmark_iter_new(containers.top()->get_node()));
    while ((ev = cmark_iter_next(iter.get())) != CMARK_EVENT_DONE)
    {
        auto node = cmark_iter_get_node(iter.get());
        if (cmark_node_get_type(node) == CMARK_NODE_DOCUMENT)
            continue;

        try
        {
            auto entity = md_entity::try_parse(result, node, *containers.top());
            assert(entity);

            if (ev == CMARK_EVENT_ENTER)
            {
                auto ptr = entity.get();
                containers.top()->add_entity(std::move(entity));

                if (is_container(ptr->get_entity_type()))
                    // new scope
                    containers.push(static_cast<md_container*>(ptr));
            }
            else if (ev == CMARK_EVENT_EXIT)
            {
                // pop scope
                containers.pop();
            }
        }
        catch (comment_parse_error& error)
        {
            p.get_logger()->warn("when parsing comments of '{}' ({}:{}): {}", name.c_str(),
                                 error.get_line(), error.get_column(), error.what());

            // reset iterator
            auto previous = cmark_node_previous(node);
            auto parent   = cmark_node_parent(node);

            cmark_node_unlink(node);
            if (previous)
                // we are exiting the previous node
                cmark_iter_reset(iter.get(), previous, CMARK_EVENT_EXIT);
            else
                // there is no previous node on this level
                // so were have just entered the parent node
                cmark_iter_reset(iter.get(), parent, CMARK_EVENT_ENTER);

            // now remove the node from the tree
            // iteration will go to the next node in order
            cmark_node_free(node);
        }
    }

    return result;
}
