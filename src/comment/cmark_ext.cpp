// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "cmark_ext.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

#include <type_safe/flag.hpp>
#include <type_safe/optional.hpp>

#include <cmark_extension_api.h>

using namespace standardese::comment;
using namespace standardese::comment::detail;

namespace
{
    // node_section_tmp() and node_inline_tmp() are temporary nodes used by parsing
    // the parser will add those as blocks that don't contain anything
    // the post process function will then merge them with the following paragraph
    cmark_node_type node_section_tmp()
    {
        static const auto type = cmark_syntax_extension_add_node(0);
        return type;
    }

    cmark_node_type node_inline_tmp()
    {
        static const auto type = cmark_syntax_extension_add_node(0);
        return type;
    }

    // initialize node types globally
    // this avoids a race condition when the first parser is created
    const auto init_nodes = (node_command(), node_inline(), node_inline_tmp(), node_section(),
                             node_section_tmp(), node_verbatim(), 0);

    //=== node manipulation commands ===//
    void set_raw_command_type(cmark_node* node, unsigned cmd)
    {
        static_assert(sizeof(void*) >= sizeof(unsigned), "fix me for your platform");
        auto as_void_ptr = reinterpret_cast<void*>(std::uintptr_t(cmd));
        cmark_node_set_user_data(node, as_void_ptr);
    }

    unsigned get_raw_command_type(cmark_node* node)
    {
        auto as_void_ptr = cmark_node_get_user_data(node);
        return unsigned(reinterpret_cast<std::uintptr_t>(as_void_ptr));
    }
}

cmark_node_type standardese::comment::detail::node_command()
{
    static const auto type = cmark_syntax_extension_add_node(0);
    return type;
}

command_type standardese::comment::detail::get_command_type(cmark_node* node)
{
    assert(cmark_node_get_type(node) == node_command());
    auto raw = get_raw_command_type(node);
    if (raw == unsigned(command_type::invalid))
        return command_type::invalid;
    else
        return make_command(raw);
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

namespace
{
    void init_node(cmark_syntax_extension* self, cmark_node* node, unsigned raw_cmd,
                   const char* arg)
    {
        cmark_node_set_syntax_extension(node, self);
        set_raw_command_type(node, raw_cmd);
        cmark_node_set_string_content(node, arg);
    }

    cmark_node* make_node(cmark_syntax_extension* self, cmark_parser* parser, cmark_node* parent,
                          int indent, cmark_node_type node_type, unsigned raw_cmd, const char* arg)
    {
        auto node = cmark_parser_add_child(parser, parent, node_type, indent);
        init_node(self, node, raw_cmd, arg);
        return node;
    }

    cmark_node* make_node(cmark_syntax_extension* self, cmark_node_type node_type, unsigned raw_cmd,
                          const char* arg)
    {
        auto node = cmark_node_new(node_type);
        init_node(self, node, raw_cmd, arg);
        return node;
    }

    //=== string parsing routines ===//
    bool is_whitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n';
    }

    bool is_special_char(char c)
    {
        // basically ispunct but allow [ and ] and _
        static auto str = R"(!\"#$%&'()*+,-./:;<=>?@\^`{|}~)";
        return std::strchr(str, c) != 0;
    }

    void skip_whitespace(char*& cur)
    {
        while (*cur && is_whitespace(*cur))
            ++cur;
    }

    std::string parse_word(char*& cur)
    {
        auto save = cur;
        skip_whitespace(cur);

        std::string word;
        for (; *cur && !is_special_char(*cur) && !is_whitespace(*cur) && *cur != '-'; ++cur)
            word += *cur;

        if (word.empty())
            cur = save;

        return word;
    }

    type_safe::optional<unsigned> try_parse_command(char*& cur, const config& c)
    {
        if (*cur == c.command_character())
        {
            ++cur;

            auto command = parse_word(cur);
            if (command.empty())
                // if command character was backslash, it was probably used to escape something
                return type_safe::nullopt;
            else
                return c.try_lookup(command.c_str());
        }
        else
            return type_safe::nullopt;
    }

    type_safe::optional<std::string> parse_section_key(char*& cur)
    {
        auto save       = cur;
        auto first_word = parse_word(cur);
        if (first_word.empty())
            return type_safe::nullopt;

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

    std::string parse_command_args(char*& cur)
    {
        skip_whitespace(cur);

        std::string result;
        while (*cur && *cur != '\n')
            result += *cur++;

        return result;
    }

    cmark_node* parse_node(cmark_syntax_extension* self, char*& cur, cmark_parser* parser,
                           cmark_node* parent, int indent)
    {
        const auto& config =
            *static_cast<standardese::comment::config*>(cmark_syntax_extension_get_private(self));

        auto save    = cur;
        auto command = try_parse_command(cur, config);
        if (!command || command.value() == unsigned(command_type::verbatim))
        {
            cur = save;
            return nullptr;
        }
        else if (is_section(command.value()))
        {
            auto key = parse_section_key(cur);
            return make_node(self, parser, parent, indent, node_section_tmp(), command.value(),
                             key.map([](const std::string& str) { return str.c_str(); })
                                 .value_or(nullptr));
        }
        else if (is_inline(command.value()))
        {
            auto entity = parse_word(cur);
            return make_node(self, parser, parent, indent, node_inline_tmp(), command.value(),
                             entity.empty() ? nullptr : entity.c_str());
        }
        else if (is_command(command.value()))
        {
            auto args = parse_command_args(cur);
            return make_node(self, parser, parent, indent, node_command(), command.value(),
                             args.c_str());
        }
        else
        {
            // add invalid command node
            // this will trigger a warning
            return make_node(self, parser, parent, indent, node_command(),
                             unsigned(command_type::invalid), std::string(save, cur).c_str());
        }
    }

    bool accept_commands(cmark_node* parent_container)
    {
        auto type = cmark_node_get_type(parent_container);
        if (type == CMARK_NODE_DOCUMENT || type == CMARK_NODE_LIST || type == node_inline())
            return true;
        else if (type == CMARK_NODE_PARAGRAPH)
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

    //=== node postprocessing ===//
    bool is_punctuation(char c)
    {
        return c == '.' || c == '!' || c == '?';
    }

    bool is_section_terminator(bool is_implicit_brief, cmark_node* node)
    {
        // terminator for implicit brief is a newline iff previous character was a punctuation
        auto prev = cmark_node_previous(node);
        if (is_implicit_brief && cmark_node_get_type(node) == CMARK_NODE_SOFTBREAK
            && cmark_node_get_type(prev) == CMARK_NODE_TEXT)
        {
            auto content = cmark_node_get_literal(prev);
            auto length  = std::strlen(content);
            if (is_punctuation(content[length - 1]))
                return true;
        }

        // linebreak is terminator for all
        return cmark_node_get_type(node) == CMARK_NODE_LINEBREAK;
    }

    cmark_node* find_section_terminator(cmark_node* container, bool is_implicit_brief)
    {
        for (auto child = cmark_node_first_child(container); child; child = cmark_node_next(child))
            if (is_section_terminator(is_implicit_brief, child))
                return child;

        return nullptr;
    }

    // returns the node that needs to be processed next
    cmark_node* split_section(cmark_node* contents, bool implicit_brief)
    {
        if (auto terminator = find_section_terminator(contents, implicit_brief))
        {
            // need to create a new node for the rest
            auto paragraph = cmark_node_new(CMARK_NODE_PARAGRAPH);
            cmark_node_insert_after(contents, paragraph);

            // add remaining nodes, after terminator
            auto cur = cmark_node_next(terminator);
            cmark_node_free(terminator);
            while (cur)
            {
                auto next = cmark_node_next(cur);
                cmark_node_append_child(paragraph, cur);
                cur = next;
            }

            return paragraph;
        }
        else
            // can keep entire section
            return cmark_node_next(contents);
    }

    cmark_node* find_prev_details(cmark_node* first, cmark_node* cur)
    {
        auto prev = cmark_node_previous(cur);
        while (prev != first
               && (cmark_node_get_type(prev) == node_command()
                   || cmark_node_get_type(prev) == node_inline()))
            prev = cmark_node_previous(prev);

        if (cmark_node_get_type(prev) == node_section()
            && get_section_type(prev) == section_type::details)
            return prev;
        else
            return nullptr;
    }

    cmark_node* wrap_in_details(cmark_syntax_extension* self, cmark_node* first, cmark_node* node)
    {
        auto details = find_prev_details(first, node);
        if (!details)
        {
            // create new details section
            details = make_node(self, node_section(), unsigned(section_type::details), nullptr);
            // insert before current one
            cmark_node_insert_before(node, details);
        }

        if (cmark_node_get_type(node) != CMARK_NODE_PARAGRAPH)
        {
            // just add entire node to details section
            auto next = cmark_node_next(node);
            cmark_node_append_child(details, node);
            return next;
        }
        else
        {
            // split section and add first part to details
            // important to split first, otherwise next will be nullptr
            auto next = split_section(node, false);
            cmark_node_append_child(details, node);
            return next;
        }
    }

    // can't use a lambda for the recursive call,
    // it changes the type of the template and will yield to infinite instantiations
    struct inline_predicate_lambda
    {
        type_safe::optional<int> last_line;
        cmark_node*              end_node;

        explicit inline_predicate_lambda(cmark_node* end_node) : end_node(end_node) {}

        bool operator()(cmark_node* node)
        {
            assert(cmark_node_get_type(node) != node_section()
                   && cmark_node_get_type(node) != node_inline());

            if (node == nullptr)
                return false;
            else if (cmark_node_get_type(node) == node_inline_tmp())
                // inlines are not allowed
                return false;
            else if (end_node)
                // only end on end node
                return node != end_node;
            else if (cmark_node_get_start_line(node) == 0)
            {
                // this node was created by splitting a parsed node
                // only accept it if it was split from a brief node
                // or the section type is the same
                auto prev = cmark_node_previous(node);
                if (cmark_node_get_type(prev) == node_section()
                    && get_section_type(prev) == section_type::brief)
                {
                    ++last_line.value();
                    return true;
                }
                else
                    return false;
            }
            else if (!last_line
                     || std::abs(last_line.value() - cmark_node_get_start_line(node)) <= 1)
            {
                // node is on the next line or same one,
                // so extend the inline section
                if (!last_line)
                    last_line = cmark_node_get_start_line(node);
                else
                    ++last_line.value();
                return true;
            }
            else
                return false;
        }
    };

    // convert temporary nodes to real ones and create implicit brief and details
    // returns the first non-processed node
    template <typename Predicate>
    cmark_node* postprocess_nodes(cmark_syntax_extension* self, cmark_node* cur,
                                  Predicate process_node)
    {
        auto first     = cur;
        auto is_inline = cmark_node_first_child(cmark_node_parent(cur)) != cur;

        type_safe::flag need_brief(true);
        while (process_node(cur))
        {
            auto next = cmark_node_next(cur);

            if (cmark_node_get_type(cur) == node_command())
            {
                if (get_command_type(cur) == command_type::end)
                {
                    assert(cur != first);

                    if (cmark_node_get_string_content(cur)[0] != '\0')
                        cmark_node_set_string_content(cur, "illegal arguments for end command");

                    // at this point all previous nodes are special nodes
                    // so just skip all details
                    auto section = cmark_node_previous(cur);
                    while (section && cmark_node_get_type(section) == node_section()
                           && get_section_type(section) == section_type::details)
                        section = cmark_node_previous(section);

                    // try to extend the current node
                    if (cmark_node_get_type(section) == node_command())
                    {
                        // it is an error to end a command
                        cmark_node_set_string_content(cur, "previous special node of end was a "
                                                           "command itself, not a section");
                    }
                    else if (!section
                             || (cmark_node_get_type(section) == node_section()
                                 && get_section_type(section) == section_type::brief))
                    {
                        // no previous section
                        cmark_node_set_string_content(cur, "no previous section for end command");
                    }
                    else
                    {
                        // add all nodes to the last section
                        for (auto in_between = cmark_node_next(section); in_between != cur;)
                        {
                            assert(get_section_type(in_between) == section_type::details);
                            auto next_in_between = cmark_node_next(in_between);

                            for (auto child = cmark_node_first_child(in_between); child;)
                            {
                                auto next_child = cmark_node_next(child);
                                auto result     = cmark_node_append_child(section, child);
                                if (!result)
                                    cmark_node_set_string_content(cur, "addding illegal children "
                                                                       "to a section");
                                child = next_child;
                            }

                            cmark_node_free(in_between);
                            in_between = next_in_between;
                        }

                        // now remove the command node
                        if (cmark_node_get_string_content(cur)[0] == '\0')
                            cmark_node_free(cur);
                    }
                }

                // don't need to do anything for other commands
            }
            else if (cmark_node_get_type(cur) == node_section_tmp())
            {
                need_brief.reset(); // don't need implicit brief
                cmark_node_set_type(cur, node_section());

                if (cmark_node_get_start_line(next) == cmark_node_get_start_line(cur)
                    || cmark_node_get_start_line(next) == cmark_node_get_start_line(cur) + 1)
                {
                    // next node can be contents of section, so add it

                    // check whether we really need current node,
                    // or whether we can extend a previous details section
                    if (get_section_type(cur) == section_type::details)
                    {
                        if (auto prev_details = find_prev_details(first, cur))
                        {
                            cmark_node_free(cur);
                            cur = prev_details;
                        }
                    }

                    auto contents = next;
                    next          = split_section(contents, false);
                    auto res      = cmark_node_append_child(cur, contents);
                    assert(res);
                }
            }
            else if (cmark_node_get_type(cur) == node_inline_tmp())
            {
                cmark_node_set_type(cur, node_inline());

                // see if there is maybe an end node
                auto end_node = cmark_node_next(cur);
                for (; end_node; end_node = cmark_node_next(end_node))
                {
                    if (cmark_node_get_type(end_node) == node_command()
                        && get_command_type(end_node) == command_type::end)
                        break;
                    else if (cmark_node_get_type(end_node) == node_section_tmp()
                             || cmark_node_get_type(end_node) == node_inline_tmp()
                             || cmark_node_get_type(end_node) == node_command())
                    {
                        // no need to look past those
                        end_node = nullptr;
                        break;
                    }
                }

                next = postprocess_nodes(self, next, inline_predicate_lambda{end_node});

                // insert all nodes in between in inline
                for (auto child = cmark_node_next(cur); child != next;)
                {
                    auto next_child = cmark_node_next(child);
                    cmark_node_append_child(cur, child);
                    child = next_child;
                }

                if (end_node)
                {
                    // don't need this one anymore
                    assert(next == end_node);
                    next = cmark_node_next(end_node);
                    cmark_node_free(end_node);
                }
            }
            else if (need_brief.try_reset() && cmark_node_get_type(cur) == CMARK_NODE_PARAGRAPH)
            {
                // create an implicit brief section
                auto brief =
                    make_node(self, node_section(), unsigned(section_type::brief), nullptr);
                cmark_node_insert_before(cur, brief);

                // add contents to it
                next = split_section(cur, true);
                cmark_node_append_child(brief, cur);
            }
            else
            {
                // can't have brief anymore
                need_brief.reset();

                auto new_next = wrap_in_details(self, first, cur);
                if (is_inline && next != new_next)
                    // we've split a section so inline is terminated
                    return new_next;
                else
                    next = new_next;
            }

            cur = next;
        }
        return cur;
    }
}

cmark_syntax_extension* standardese::comment::detail::create_command_extension(config& c)
{
    (void)init_nodes;

    auto ext = cmark_syntax_extension_new("standardese_commands");
    cmark_syntax_extension_set_private(ext, &c, [](cmark_mem*, void*) {});

    cmark_syntax_extension_set_get_type_string_func(ext, [](cmark_syntax_extension*,
                                                            cmark_node* node) {
        if (cmark_node_get_type(node) == node_command())
            return "standardese_command";
        else if (cmark_node_get_type(node) == node_section())
            return "standardese_section";
        else if (cmark_node_get_type(node) == node_section_tmp())
            return "__standardese_section";
        else if (cmark_node_get_type(node) == node_inline())
            return "standardese_inline";
        else if (cmark_node_get_type(node) == node_inline_tmp())
            return "__standardese_inline";
        else
            return "<unknown>";
    });
    auto can_contain = [](cmark_syntax_extension*, cmark_node* node,
                          cmark_node_type child_type) -> int {
        auto node_type = cmark_node_get_type(node);
        if (node_type == node_command() || node_type == node_inline_tmp()
            || node_type == node_section_tmp())
            return false;
        else if (node_type == node_section())
        {
            if (get_section_type(node) == section_type::details)
                // can contain any block
                return (child_type & CMARK_NODE_TYPE_MASK) == CMARK_NODE_TYPE_BLOCK;
            else
                // can only contain paragraphs
                return child_type == CMARK_NODE_PARAGRAPH;
        }
        else if (cmark_node_get_type(node) == node_inline())
            // can contain anything except other inlines
            return child_type != node_inline();
        else
            return false;
    };
    cmark_syntax_extension_set_can_contain_func(ext, can_contain);

    cmark_syntax_extension_set_open_block_func(
        ext,
        [](cmark_syntax_extension* self, int indented, cmark_parser* parser,
           cmark_node* parent_container, unsigned char* input, int len) -> cmark_node* {
            // if we have a command character,
            // create an inline node consuming as much as possible,
            // then return a paragraph containing this command

            if (!accept_commands(parent_container))
                return nullptr;

            assert(cmark_parser_get_offset(parser) <= len);
            auto real_input = reinterpret_cast<char*>(input) + cmark_parser_get_offset(parser);

            auto cur  = real_input;
            auto node = parse_node(self, cur, parser, parent_container, indented);
            cmark_parser_advance_offset(parser, reinterpret_cast<char*>(input),
                                        static_cast<int>(cur - real_input), false);
            return node;
        });

    cmark_syntax_extension_set_postprocess_func(ext,
                                                [](cmark_syntax_extension* self, cmark_parser*,
                                                   cmark_node*             root) -> cmark_node* {
                                                    postprocess_nodes(self,
                                                                      cmark_node_first_child(root),
                                                                      [](cmark_node* node) {
                                                                          return node != nullptr;
                                                                      });
                                                    return nullptr;
                                                });

    return ext;
}

namespace
{
    bool bump_if(cmark_inline_parser* parser, char cmd_char, const char* str)
    {
        auto offset = cmark_inline_parser_get_offset(parser);
        if (offset == 0 || cmark_inline_parser_peek_at(parser, offset - 1) != cmd_char)
            return false;

        while (*str)
        {
            if (cmark_inline_parser_peek_at(parser, offset) != *str)
                return false;
            ++str;
            ++offset;
        }

        cmark_inline_parser_set_offset(parser, offset);
        return true;
    }

    bool is_space(char c)
    {
        return c == ' ' || c == '\t';
    }

    void trim_spaces(std::string& str)
    {
        auto begin_offset = 0u;
        while (begin_offset < str.size() && is_space(str[begin_offset]))
            ++begin_offset;

        str = str.substr(begin_offset);
        while (!str.empty() && is_space(str.back()))
            str.pop_back();
    }

    cmark_node* parse_verbatim(cmark_syntax_extension* self, cmark_parser*, cmark_node* parent,
                               unsigned char, cmark_inline_parser* inline_parser)
    {
        const auto& config =
            *static_cast<standardese::comment::config*>(cmark_syntax_extension_get_private(self));

        if (bump_if(inline_parser, config.command_character(),
                    config.command_name(command_type::verbatim)))
        {
            cmark_node_unput(parent, 1); // remove command character

            std::string content;

            auto break_due_to_eof = false;
            while (!bump_if(inline_parser, config.command_character(),
                            config.command_name(command_type::end)))
            {
                content += char(cmark_inline_parser_peek_char(inline_parser));
                if (cmark_inline_parser_is_eof(inline_parser))
                {
                    break_due_to_eof = true;
                    break;
                }
                cmark_inline_parser_advance_offset(inline_parser);
            }

            if (!break_due_to_eof)
            {
                assert(!content.empty() && content.back() == config.command_character());
                content.pop_back();
            }

            trim_spaces(content);

            auto node = cmark_node_new(node_verbatim());
            cmark_node_set_string_content(node, content.c_str());
            cmark_node_set_syntax_extension(node, self);
            return node;
        }
        else
            return nullptr;
    }
}

cmark_node_type standardese::comment::detail::node_verbatim()
{
    static const auto type = cmark_syntax_extension_add_node(1);
    return type;
}

const char* standardese::comment::detail::get_verbatim_content(cmark_node* node)
{
    assert(cmark_node_get_type(node) == node_verbatim());
    return cmark_node_get_string_content(node);
}

cmark_syntax_extension* standardese::comment::detail::create_verbatim_extension(config& c)
{
    auto ext = cmark_syntax_extension_new("standardese_verbatim");

    cmark_syntax_extension_set_private(ext, &c, [](cmark_mem*, void*) {});
    cmark_syntax_extension_set_get_type_string_func(ext, [](cmark_syntax_extension*,
                                                            cmark_node* node) {
        if (cmark_node_get_type(node) == node_verbatim())
            return "standardese_verbatim";
        else
            return "<unknown>";
    });

    cmark_syntax_extension_set_special_inline_chars(
        ext, cmark_llist_append(cmark_get_default_mem_allocator(), nullptr,
                                reinterpret_cast<void*>('\\')));
    cmark_syntax_extension_set_match_inline_func(ext, &parse_verbatim);

    return ext;
}

cmark_syntax_extension* standardese::comment::detail::create_no_html_extension()
{
    auto ext = cmark_syntax_extension_new("standardese_no_html");
    cmark_syntax_extension_set_postprocess_func(
        ext, [](cmark_syntax_extension* self, cmark_parser*, cmark_node* root) -> cmark_node* {
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
