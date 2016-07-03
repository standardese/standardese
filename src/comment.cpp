// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment.hpp>

#include <cmark.h>
#include <stack>

#include <standardese/detail/wrapper.hpp>
#include <standardese/error.hpp>
#include <standardese/md_blocks.hpp>
#include <standardese/md_inlines.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

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
        while (*ptr == ' ' || *ptr == '\t')
            ++ptr;

        if (*ptr == '/')
        {
            while (*ptr == '/')
                ++ptr;

            while (*ptr == ' ' || *ptr == '\t')
                ++ptr;
        }
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
                skip_comment(++cur);
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

    void remove_node(cmark_node* node, cmark_iter* iter)
    {
        // reset iterator
        auto previous = cmark_node_previous(node);
        auto parent   = cmark_node_parent(node);

        cmark_node_unlink(node);
        if (previous)
            // we are exiting the previous node
            cmark_iter_reset(iter, previous, CMARK_EVENT_EXIT);
        else
            // there is no previous node on this level
            // so were have just entered the parent node
            cmark_iter_reset(iter, parent, CMARK_EVENT_ENTER);

        // now remove the node from the tree
        // iteration will go to the next node in order
        cmark_node_free(node);
    }

    void add_entity(std::stack<md_container*>& containers, md_entity_ptr entity)
    {
        assert(entity);

        auto ptr = entity.get();
        containers.top()->add_entity(std::move(entity));

        if (is_container(ptr->get_entity_type()))
            // new scope
            containers.push(static_cast<md_container*>(ptr));
    }

    void parse_command(const parser& p, md_paragraph& paragraph, bool& first)
    {
        // set implicit section type
        auto def_section = first ? section_type::brief : section_type::details;
        paragraph.set_section_type(def_section,
                                   p.get_output_config().get_section_name(def_section));
        first = false;

        if (paragraph.begin()->get_entity_type() != md_entity::text_t)
            // a raw text as first child for section is required
            // i.e. not emphasis or similar
            return;

        auto& text = static_cast<md_text&>(*paragraph.begin());

        auto str = text.get_string();
        if (*str != p.get_comment_config().get_command_character())
            // require command at first place
            return;
        ++str;

        // read until whitespace
        std::string command;
        while (!std::isspace(*str))
            command += *str++;

        auto section = p.get_comment_config().get_section(command);
        if (section == section_type::invalid)
            throw comment_parse_error("Unknown command '" + command + "'",
                                      cmark_node_get_start_line(paragraph.get_node()),
                                      cmark_node_get_start_column(paragraph.get_node()));

        // remove command + command character + whitespace
        auto new_str = text.get_string() + command.size() + 1;
        while (std::isspace(*new_str))
            ++new_str;

        // need a copy, cmark can't handle it otherwise, https://github.com/jgm/cmark/issues/139
        text.set_string(std::string(new_str).c_str());

        paragraph.set_section_type(section, p.get_output_config().get_section_name(section));
    }

    void parse_children(comment& result, md_document& document, const parser& p,
                        const cpp_name& name)
    {
        std::stack<md_container*> containers;
        containers.push(&document);

        auto    ev              = CMARK_EVENT_NONE;
        auto    first_paragraph = true;
        md_iter iter(cmark_iter_new(containers.top()->get_node()));
        while ((ev = cmark_iter_next(iter.get())) != CMARK_EVENT_DONE)
        {
            auto node = cmark_iter_get_node(iter.get());
            if (cmark_node_get_type(node) == CMARK_NODE_DOCUMENT)
                // skip document, already parsed
                continue;

            try
            {
                if (ev == CMARK_EVENT_ENTER)
                {
                    auto entity = md_entity::try_parse(result, node, *containers.top());
                    add_entity(containers, std::move(entity));
                }
                else if (ev == CMARK_EVENT_EXIT)
                {
                    // pop scope
                    auto top = containers.top();
                    containers.pop();

                    if (top->get_entity_type() == md_entity::paragraph_t)
                        parse_command(p, *static_cast<md_paragraph*>(top), first_paragraph);
                }
            }
            catch (comment_parse_error& error)
            {
                p.get_logger()->warn("when parsing comments of '{}' ({}:{}): {}", name.c_str(),
                                     error.get_line(), error.get_column(), error.what());
                remove_node(node, iter.get());
            }
        }
    }
}

comment comment::parse(const parser& p, const cpp_name& name, const cpp_raw_comment& raw_comment)
{
    comment result(parse_document(raw_comment));
    parse_children(result, *result.document_, p, name);
    return result;
}
