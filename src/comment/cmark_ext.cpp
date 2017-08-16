// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "cmark_ext.hpp"

#include <cassert>
#include <cstdint>
#include <string>

#include <type_safe/flag.hpp>
#include <type_safe/optional.hpp>

#include <cmark_extension_api.h>

using namespace standardese::comment;
using namespace standardese::comment::detail;

namespace
{
    void set_raw_command_type(cmark_node* node, unsigned cmd)
    {
        static_assert(sizeof(void*) >= sizeof(unsigned), "fix me for your platform");
        auto as_void_ptr = reinterpret_cast<void*>(cmd);
        cmark_node_set_user_data(node, as_void_ptr);
    }

    unsigned get_raw_command_type(cmark_node* node)
    {
        auto as_void_ptr = cmark_node_get_user_data(node);
        return unsigned(reinterpret_cast<std::uintptr_t>(as_void_ptr));
    }

    bool is_whitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n';
    }

    void skip_whitespace(char*& cur)
    {
        while (*cur && is_whitespace(*cur))
            ++cur;
    }

    std::string parse_word(char*& cur)
    {
        skip_whitespace(cur);

        std::string word;
        for (; *cur && !is_whitespace(*cur) && *cur != '-'; ++cur)
            word += *cur;

        return word;
    }

    unsigned try_parse_command(char*& cur, const config& c)
    {
        if (*cur == c.command_character())
        {
            ++cur;

            auto command = parse_word(cur);
            return c.try_lookup(command.c_str());
        }
        else
            return unsigned(command_type::invalid);
    }

    bool accept_commands(cmark_node* parent_container)
    {
        if (cmark_node_get_type(parent_container) == CMARK_NODE_DOCUMENT)
            return true;
        else if (cmark_node_get_type(parent_container) == CMARK_NODE_PARAGRAPH)
        {
            // allow paragraphs if parent is either:
            // * document (happens when command on second line in paragraph)
            // * inline (happens when command in inline)
            auto parent_parent_type = cmark_node_get_type(cmark_node_parent(parent_container));
            return parent_parent_type == CMARK_NODE_DOCUMENT || parent_parent_type == node_inline();
        }
        else
            return false;
    }

    void set_node(cmark_syntax_extension* self, cmark_node* node, unsigned command, const char* str)
    {
        cmark_node_set_syntax_extension(node, self);
        set_raw_command_type(node, command);
        cmark_node_set_string_content(node, str);
    }

    cmark_node* create_node(cmark_syntax_extension* self, int indent, cmark_parser* parser,
                            cmark_node* parent, cmark_node_type type, unsigned command,
                            const char* str)
    {
        auto node = cmark_parser_add_child(parser, parent, type, indent);
        set_node(self, node, command, str);
        return node;
    }

    cmark_node* create_node(cmark_syntax_extension* self, cmark_node_type type, unsigned command,
                            const char* str)
    {
        auto node = cmark_node_new(type);
        set_node(self, node, command, str);
        return node;
    }

    type_safe::optional<std::string> parse_section_key(char*& cur)
    {
        auto save       = cur;
        auto first_word = parse_word(cur);
        skip_whitespace(cur);
        if (*cur == '-')
        {
            // is a key - value section
            ++cur;
            skip_whitespace(cur);

            return first_word;
        }
        else
            // don't have a key
            cur = save;

        return type_safe::nullopt;
    }

    cmark_node* try_open_block(cmark_syntax_extension* self, int indent, cmark_parser* parser,
                               cmark_node* parent_container, unsigned char* input, int len)
    {
        if (!accept_commands(parent_container))
            return nullptr;

        const auto& config =
            *static_cast<standardese::comment::config*>(cmark_syntax_extension_get_private(self));
        auto cur     = reinterpret_cast<char*>(input);
        auto command = try_parse_command(cur, config);
        if (is_section(command))
        {
            auto node =
                create_node(self, indent, parser, parent_container, node_section(), command,
                            parse_section_key(cur).map(&std::string::c_str).value_or(nullptr));

            // skip command
            cmark_parser_advance_offset(parser, reinterpret_cast<char*>(input),
                                        int(cur - reinterpret_cast<char*>(input)), 0);

            return node;
        }
        else if (is_command(command))
        {
            // skip rest of line
            skip_whitespace(cur);
            cmark_parser_advance_offset(parser, reinterpret_cast<char*>(input), len, 0);

            // I don't want a trailing newline, so override it with the terminating null
            // hey, if I get non-const strings, I write to them!
            input[len - 1] = 0;
            return create_node(self, indent, parser, parent_container, node_command(), command,
                               cur); // store remainder of line as arguments
        }
        else if (is_inline(command))
        {
            auto entity = parse_word(cur);
            if (*cur == '\n')
                // skip linemarker as well
                ++cur;
            auto node = create_node(self, indent, parser, parent_container, node_inline(), command,
                                    entity.c_str());

            // skip command
            cmark_parser_advance_offset(parser, reinterpret_cast<char*>(input),
                                        int(cur - reinterpret_cast<char*>(input)), 0);

            return node;
        }
        else
            return nullptr;
    }

    int last_block_matches(cmark_syntax_extension* self, cmark_parser*, unsigned char* input, int,
                           cmark_node* container)
    {
        if (cmark_node_get_type(container) != node_inline())
            // only inlines can grow
            return false;

        const auto& config =
            *static_cast<standardese::comment::config*>(cmark_syntax_extension_get_private(self));
        if (*input == config.command_character())
            // allow commands, if it happens to be another section/inline,
            // the contain function won't accept it and it will be closed
            return true;
        // accept anything else only if empty or last node was command
        // this allows paragraph after commands
        // if node won't become a paragraph, contain won't accept it
        return !cmark_node_last_child(container)
               || cmark_node_get_type(cmark_node_last_child(container)) == node_command();
    }

    // moves all commands and inlines to the back
    void sort_special_nodes(cmark_node* root)
    {
        cmark_node* first_command = nullptr;
        cmark_node* first_inline  = nullptr;
        for (auto cur = cmark_node_first_child(root); cur != first_command && cur != first_inline;)
        {
            if (cmark_node_get_type(cur) == node_command())
            {
                auto next = cmark_node_next(cur);
                cmark_node_unlink(cur);
                if (first_command)
                    // insert after first command
                    // relative order of command doesn't matter
                    cmark_node_insert_after(first_command, cur);
                else if (first_inline)
                {
                    // first command, but already got an inline
                    // insert before the inline
                    cmark_node_insert_before(first_inline, cur);
                    first_command = cur;
                }
                else
                {
                    // neither command nor inline before,
                    // so just append at the back
                    cmark_node_append_child(root, cur);
                    first_command = cur;
                }
                cur = next;
            }
            else if (cmark_node_get_type(cur) == node_inline())
            {
                auto next = cmark_node_next(cur);
                cmark_node_unlink(cur);

                cmark_node_append_child(root, cur);
                if (!first_inline)
                    first_inline = cur;

                cur = next;
            }
            else
                cur = cmark_node_next(cur);
        }
    }

    cmark_node* prev_details(cmark_node* cur)
    {
        auto details = cmark_node_previous(cur);
        return details && cmark_node_get_type(details) == node_section()
                       && detail::get_section_type(details) == section_type::details ?
                   details :
                   nullptr;
    }

    // terminates a section or inline on a terminator
    void terminate_section(cmark_syntax_extension* self, cmark_node* section,
                           cmark_node* terminator)
    {
        assert(cmark_node_get_type(section) == node_section()
               || cmark_node_get_type(section) == node_inline());

        // remainder of paragraph is considered details
        auto detail_node = prev_details(section);
        if (!detail_node)
        {
            detail_node =
                create_node(self, node_section(), unsigned(section_type::details), nullptr);
            cmark_node_insert_after(section, detail_node);
        }

        auto paragraph = cmark_node_new(CMARK_NODE_PARAGRAPH);
        cmark_node_append_child(detail_node, paragraph);

        // add remaining children to paragraph
        for (auto cur = cmark_node_next(terminator); cur;)
        {
            auto next = cmark_node_next(cur);
            cmark_node_append_child(paragraph, cur);
            cur = next;
        }

        // delete terminator
        cmark_node_free(terminator);
    }

    // returns the terminator of the brief section
    cmark_node* brief_terminator(cmark_node* paragraph)
    {
        assert(cmark_node_get_type(paragraph) == CMARK_NODE_PARAGRAPH);

        for (auto child = cmark_node_first_child(paragraph); child; child = cmark_node_next(child))
            if (cmark_node_get_type(child) == CMARK_NODE_SOFTBREAK
                || cmark_node_get_type(child) == CMARK_NODE_LINEBREAK)
                return child;

        return nullptr;
    }

    cmark_node* wrap_in_details(cmark_syntax_extension* self, cmark_node* cur)
    {
        auto details = prev_details(cur);
        if (!details)
        {
            // create new detail section
            details = create_node(self, node_section(), unsigned(section_type::details), nullptr);
            cmark_node_replace(cur, details);
        }

        cmark_node_append_child(details, cur);
        return details;
    }

    // finds a line break in a section/inline
    cmark_node* find_linebreak(cmark_node* section)
    {
        assert(cmark_node_get_type(section) == node_section()
               || cmark_node_get_type(section) == node_inline());
        auto paragraph = cmark_node_first_child(section);
        assert(!paragraph || cmark_node_get_type(paragraph) == CMARK_NODE_PARAGRAPH);
        if (!paragraph)
            return nullptr;

        for (auto cur = cmark_node_first_child(paragraph); cur; cur = cmark_node_next(cur))
            if (cmark_node_get_type(cur) == CMARK_NODE_LINEBREAK)
                return cur;
        return nullptr;
    }

    // post process function
    cmark_node* create_implicit_brief_details(cmark_syntax_extension* self, cmark_parser*,
                                              cmark_node*             root)
    {
        sort_special_nodes(root);

        type_safe::flag need_brief(true);
        for (auto cur = cmark_node_first_child(root); cur; cur = cmark_node_next(cur))
        {
            if (need_brief.try_reset() && cmark_node_get_type(cur) == CMARK_NODE_PARAGRAPH)
            {
                // create implicit brief
                auto node =
                    create_node(self, node_section(), unsigned(section_type::brief), nullptr);

                cmark_node_replace(cur, node);
                cmark_node_append_child(node, cur);

                // terminate brief section at first line break
                if (auto terminator = brief_terminator(cur))
                    terminate_section(self, node, terminator);

                cur = node;
            }
            else if (cmark_node_get_type(cur) == node_section()
                     && detail::get_section_type(cur) == section_type::brief)
            {
                need_brief.reset(); // don't need brief anymore
            }
            else if (cmark_node_get_type(cur) == node_section()
                     && detail::get_section_type(cur) != section_type::details)
            {
                if (auto linebreak = find_linebreak(cur))
                    terminate_section(self, cur, linebreak);
            }
            else if (cmark_node_get_type(cur) == node_inline())
            {
                create_implicit_brief_details(self, nullptr, cur);
            }
            else if (cmark_node_get_type(cur) != node_section()
                     && cmark_node_get_type(cur) != node_command())
            {
                need_brief.reset(); // no implicit brief possible anymore
                cur = wrap_in_details(self, cur);
            }
        }

        // don't have a new root node
        return nullptr;
    }

    // initialize node types globally
    // this avoids a race condition when the first parser is created
    const auto init_nodes = (node_command(), node_inline(), node_section(), 0);
}

cmark_syntax_extension* standardese::comment::detail::create_command_extension(config& c)
{
    (void)init_nodes;

    auto ext = cmark_syntax_extension_new("standardese_command");

    cmark_syntax_extension_set_get_type_string_func(ext, [](cmark_syntax_extension*,
                                                            cmark_node* node) {
        if (cmark_node_get_type(node) == node_command())
            return "standardese_command";
        else if (cmark_node_get_type(node) == node_section())
            return "standardese_section";
        else if (cmark_node_get_type(node) == node_inline())
            return "standardese_inline";
        else
            return "<unknown>";
    });
    cmark_syntax_extension_set_can_contain_func(ext, [](cmark_syntax_extension*, cmark_node* node,
                                                        cmark_node_type child_type) -> int {
        if (cmark_node_get_type(node) == node_command())
            return false;
        else if (cmark_node_get_type(node) == node_section())
        {
            if (get_section_type(node) == section_type::details)
                // can contain any block
                return (child_type & CMARK_NODE_TYPE_MASK) == CMARK_NODE_TYPE_BLOCK;
            else
                // can only contain paragraphs
                return child_type == CMARK_NODE_PARAGRAPH;
        }
        else if (cmark_node_get_type(node) == node_inline())
            // can contain paragraphs, sections or commands
            // first it will only contain paragraphs or commands,
            // but postprocessing will add brief and details sections,
            // so it won't contain paragraphs anymore
            return child_type == CMARK_NODE_PARAGRAPH || child_type == node_section()
                   || child_type == node_command();
        else
            return false;
    });
    cmark_syntax_extension_set_open_block_func(ext, &try_open_block);
    cmark_syntax_extension_set_match_block_func(ext, &last_block_matches);
    cmark_syntax_extension_set_postprocess_func(ext, &create_implicit_brief_details);

    cmark_syntax_extension_set_private(ext, &c, [](cmark_mem*, void*) {});

    return ext;
}

cmark_node_type standardese::comment::detail::node_command()
{
    static const auto type = cmark_syntax_extension_add_node(0);
    return type;
}

command_type standardese::comment::detail::get_command_type(cmark_node* node)
{
    assert(cmark_node_get_type(node) == node_command());
    return make_command(get_raw_command_type(node));
}

const char* standardese::comment::detail::get_command_arguments(cmark_node* node)
{
    assert(cmark_node_get_type(node) == node_command());
    return cmark_node_get_string_content(node);
}

cmark_node_type standardese::comment::detail::node_section()
{
    static const auto type = cmark_syntax_extension_add_node(0);
    return type;
}

section_type standardese::comment::detail::get_section_type(cmark_node* node)
{
    assert(cmark_node_get_type(node) == node_section());
    return make_section(get_raw_command_type(node));
}

const char* standardese::comment::detail::get_section_key(cmark_node* node)
{
    assert(cmark_node_get_type(node) == node_section());
    auto content = cmark_node_get_string_content(node);
    return *content == '\0' ? nullptr : content;
}

cmark_node_type standardese::comment::detail::node_inline()
{
    static const auto type = cmark_syntax_extension_add_node(0);
    return type;
}

inline_type standardese::comment::detail::get_inline_type(cmark_node* node)
{
    assert(cmark_node_get_type(node) == node_inline());
    return make_inline(get_raw_command_type(node));
}

const char* standardese::comment::detail::get_inline_entity(cmark_node* node)
{
    assert(cmark_node_get_type(node) == node_inline());
    return cmark_node_get_string_content(node);
}

cmark_syntax_extension* standardese::comment::detail::create_no_html_extension()
{
    auto ext = cmark_syntax_extension_new("standardese_no_html");
    cmark_syntax_extension_set_postprocess_func(ext, [](cmark_syntax_extension* self, cmark_parser*,
                                                        cmark_node* root) -> cmark_node* {
        auto iter = cmark_iter_new(root);

        auto ev = CMARK_EVENT_NONE;
        while ((ev = cmark_iter_next(iter)) != CMARK_EVENT_DONE)
        {
            if (ev == CMARK_EVENT_EXIT)
                continue;

            auto node = cmark_iter_get_node(iter);
            if (cmark_node_get_type(node) == CMARK_NODE_HTML_INLINE)
            {
                cmark_node* text;

                auto prev = cmark_node_previous(node);
                if (prev && cmark_node_get_type(prev) == CMARK_NODE_TEXT)
                {
                    text = prev;

                    // prepend to previous text
                    std::string content = cmark_node_get_literal(text);
                    content += cmark_node_get_literal(node);
                    cmark_node_set_literal(text, content.c_str());
                }
                else
                {
                    // create new text and replace node
                    text = cmark_node_new(CMARK_NODE_TEXT);
                    cmark_node_set_syntax_extension(text, self);
                    cmark_node_set_literal(text, cmark_node_get_literal(node));

                    cmark_node_replace(node, text);
                }

                cmark_node_free(node);

                auto next = cmark_node_next(text);
                if (next && cmark_node_get_type(next) == CMARK_NODE_TEXT)
                {
                    // merge with following text node
                    std::string content = cmark_node_get_literal(text);
                    content += cmark_node_get_literal(next);
                    cmark_node_set_literal(text, content.c_str());

                    cmark_node_free(next);
                }

                cmark_iter_reset(iter, text, CMARK_EVENT_ENTER);
            }
        }

        cmark_iter_free(iter);
        return nullptr; // no new root
    });
    return ext;
}
