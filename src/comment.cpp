// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment.hpp>

#include <cmark.h>
#include <stack>

#include <standardese/detail/wrapper.hpp>
#include <standardese/error.hpp>
#include <standardese/generator.hpp>
#include <standardese/md_blocks.hpp>
#include <standardese/md_inlines.hpp>
#include <standardese/parser.hpp>
#include <standardese/section.hpp>

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

    cmark_node* parse_document(const parser& p, const string& raw_comment)
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

                if (p.get_comment_config().get_implicit_paragraph())
                {
                    // add empty line to finish paragraph
                    char newline = '\n';
                    cmark_parser_feed(parser.get(), &newline, 1);
                }
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

        return cmark_parser_finish(parser.get());
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

    std::string read_command(const parser& p, md_paragraph& paragraph)
    {
        if (paragraph.begin()->get_entity_type() != md_entity::text_t)
            // a raw text as first child for section is required
            // i.e. not emphasis or similar
            return "";

        auto str = static_cast<md_text&>(*paragraph.begin()).get_string();
        if (*str != p.get_comment_config().get_command_character())
            // require command at first place
            return "";
        ++str;

        // read until whitespace
        std::string command;
        while (*str && !std::isspace(*str))
            command += *str++;

        return command;
    }

    md_text& remove_command_string(md_paragraph& paragraph, const std::string& command)
    {
        // remove command + command character + whitespace
        auto& text    = static_cast<md_text&>(*paragraph.begin());
        auto  new_str = text.get_string() + command.size() + 1;
        while (std::isspace(*new_str))
            ++new_str;

        // need a copy, cmark can't handle it otherwise, https://github.com/jgm/cmark/issues/139
        text.set_string(std::string(new_str).c_str());

        return text;
    }

    void parse_command(const parser& p, md_comment& comment, md_paragraph& paragraph, bool& first)
    {
        // set implicit section type
        auto def_section = first ? section_type::brief : section_type::details;
        paragraph.set_section_type(def_section,
                                   p.get_output_config().get_section_name(def_section));
        first = false;

        auto command = read_command(p, paragraph);
        if (command.empty())
            return;
        auto& text = remove_command_string(paragraph, command);

        auto c = p.get_comment_config().try_get_command(command);
        if (is_section(c))
            paragraph.set_section_type(make_section(c),
                                       p.get_output_config().get_section_name(make_section(c)));
        else if (make_command(c) == command_type::exclude)
            comment.set_excluded(true);
        else if (make_command(c) == command_type::unique_name)
        {
            comment.set_unique_name(text.get_string());
            text.set_string(""); // might need to actually delete the text node at some point
        }
        else
            throw comment_parse_error("Unknown command '" + command + "'",
                                      cmark_node_get_start_line(paragraph.get_node()),
                                      cmark_node_get_start_column(paragraph.get_node()));
    }

    void parse_children(md_comment& comment, const parser& p, cmark_node* root,
                        const cpp_name& name)
    {
        std::stack<md_container*> containers;
        containers.push(&comment);

        std::vector<md_entity_ptr> direct_children;

        auto    ev              = CMARK_EVENT_NONE;
        auto    first_paragraph = true;
        md_iter iter(cmark_iter_new(root));
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
                    auto entity = md_entity::try_parse(node, *containers.top());
                    auto parent = containers.top();

                    if (is_container(entity->get_entity_type()))
                        containers.push(static_cast<md_container*>(entity.get()));

                    if (cmark_node_parent(node) == root)
                        direct_children.push_back(std::move(entity));
                    else
                        parent->add_entity(std::move(entity));
                }
                else if (ev == CMARK_EVENT_EXIT)
                {
                    // pop scope
                    auto top = containers.top();
                    containers.pop();

                    if (top->get_entity_type() == md_entity::paragraph_t)
                        parse_command(p, comment, *static_cast<md_paragraph*>(top),
                                      first_paragraph);
                }
            }
            catch (comment_parse_error& error)
            {
                p.get_logger()->warn("when parsing comments of '{}' ({}:{}): {}", name.c_str(),
                                     error.get_line(), error.get_column(), error.what());

                if (cmark_node_parent(node) == root)
                {
                    // if the parant is root, remove from direct_children as well
                    assert(direct_children.back()->get_node() == node);
                    direct_children.pop_back();
                }
                remove_node(node, iter.get());
            }
        }

        for (auto& e : direct_children)
            comment.add_entity(std::move(e));
    }
}

md_ptr<md_comment> md_comment::parse(const parser& p, const string& name, const string& comment)
{
    auto result = detail::make_md_ptr<md_comment>();

    auto root = parse_document(p, comment);
    parse_children(*result, p, root, name);
    cmark_node_free(root);

    return result;
}

md_entity_ptr md_comment::do_clone(const md_entity*) const
{
    auto result       = detail::make_md_ptr<md_comment>();
    result->excluded_ = excluded_;
    result->id_       = id_;
    for (auto& child : *this)
        result->add_entity(child.clone(*result));
    return std::move(result);
}

md_comment::md_comment()
: md_container(get_entity_type(), cmark_node_new(CMARK_NODE_CUSTOM_BLOCK)), excluded_(false)
{
}

namespace
{
    const md_document* get_document(const md_entity* cur) STANDARDESE_NOEXCEPT
    {
        while (cur->has_parent() && cur->get_entity_type() != md_entity::document_t)
            cur = &cur->get_parent();

        if (cur->get_entity_type() == md_entity::document_t)
            return static_cast<const md_document*>(cur);
        else
            return nullptr;
    }
}

std::string md_comment::get_output_name() const
{
    auto document = get_document(this);
    return document ? document->get_output_name() : "";
}
