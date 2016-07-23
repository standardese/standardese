// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment.hpp>

#include <cmark.h>
#include <stack>

#include <standardese/detail/raw_comment.hpp>
#include <standardese/detail/wrapper.hpp>
#include <standardese/error.hpp>
#include <standardese/generator.hpp>
#include <standardese/md_blocks.hpp>
#include <standardese/md_inlines.hpp>
#include <standardese/parser.hpp>
#include <standardese/section.hpp>

using namespace standardese;

md_ptr<md_comment> md_comment::make()
{
    return detail::make_md_ptr<md_comment>();
}

md_entity& md_comment::add_entity(md_entity_ptr ptr)
{
    if (ptr->get_entity_type() == md_entity::paragraph_t)
    {
        auto& par = static_cast<md_paragraph&>(*ptr);
        if (par.get_section_type() == section_type::brief)
        {
            // add all children to brief
            for (auto& child : par)
                get_brief().add_entity(child.clone(get_brief()));
            return get_brief();
        }
    }

    return md_container::add_entity(std::move(ptr));
}

md_entity_ptr md_comment::do_clone(const md_entity*) const
{
    auto result = md_comment::make();
    for (auto& child : *this)
        result->add_entity(child.clone(*result));
    return std::move(result);
}

md_comment::md_comment() : md_container(get_entity_type(), cmark_node_new(CMARK_NODE_CUSTOM_BLOCK))
{
    auto brief = md_paragraph::make(*this);
    brief->set_section_type(section_type::brief, "");

    md_container::add_entity(std::move(brief));
}

bool detail::comment_compare::operator()(const comment_id& id_a,
                                         const comment_id& id_b) const STANDARDESE_NOEXCEPT
{
    if (id_a.file_name_or_name_ != id_b.file_name_or_name_)
        return id_a.file_name_or_name_ < id_b.file_name_or_name_;
    return id_a.line_ < id_b.line_;
}

bool comment_registry::register_comment(comment_id id, comment c) const
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto result = comments_.insert(std::make_pair(std::move(id), std::move(c)));
    return result.second;
}

namespace
{
    comment_id create_location_id(const cpp_entity& e)
    {
        if (e.get_entity_type() == cpp_entity::file_t)
            return comment_id(e.get_full_name());

        auto location = clang_getCursorLocation(e.get_cursor());

        CXFile   file;
        unsigned line;
        clang_getSpellingLocation(location, &file, &line, nullptr, nullptr);
        assert(file);

        return line == 1 ? comment_id("", 1) : comment_id(clang_getFileName(file), line - 1);
    }

    comment_id create_name_id(const cpp_entity& e)
    {
        return comment_id(e.get_unique_name());
    }
}

const comment* comment_registry::lookup_comment(const cpp_entity& e) const
{
    std::unique_lock<std::mutex> lock(mutex_);

    auto location = create_location_id(e);
    auto iter     = comments_.lower_bound(location);
    if (iter != comments_.end()
        && (location.is_name() || location.line() + 1 - iter->first.line() <= 1))
        return &iter->second;

    iter = comments_.find(create_name_id(e));
    if (iter != comments_.end())
        return &iter->second;

    return nullptr;
}

namespace
{
    struct node_deleter
    {
        void operator()(cmark_node* node) const STANDARDESE_NOEXCEPT
        {
            cmark_node_free(node);
        }
    };

    using md_node = detail::wrapper<cmark_node*, node_deleter>;

    md_node parse_document(const parser& p, const string& raw_comment)
    {
        struct parser_deleter
        {
            void operator()(cmark_parser* parser) const STANDARDESE_NOEXCEPT
            {
                cmark_parser_free(parser);
            }
        };

        using md_parser = detail::wrapper<cmark_parser*, parser_deleter>;

        md_parser parser(cmark_parser_new(CMARK_OPT_NORMALIZE));

        std::string cur;
        for (auto c : raw_comment)
        {
            if (c == '\n')
            {
                cur += '\n';
                if (p.get_comment_config().get_implicit_paragraph())
                    cur += '\n'; // add empty newline
                cmark_parser_feed(parser.get(), cur.c_str(), cur.length());
                cur.clear();
            }
            else
                cur += c;
        }
        cmark_parser_feed(parser.get(), cur.c_str(), cur.length());

        return cmark_parser_finish(parser.get());
    }

    std::vector<md_entity_ptr> parse_children(comment& comment, const md_node& root)
    {
        struct iter_deleter
        {
            void operator()(cmark_iter* iter) const STANDARDESE_NOEXCEPT
            {
                cmark_iter_free(iter);
            }
        };

        using md_iter = detail::wrapper<cmark_iter*, iter_deleter>;

        std::vector<md_entity_ptr> direct_children;
        std::stack<md_container*>  stack;
        stack.push(&comment.get_content());

        md_iter iter(cmark_iter_new(root.get()));
        for (auto ev = CMARK_EVENT_NONE; (ev = cmark_iter_next(iter.get())) != CMARK_EVENT_DONE;)
        {
            auto node = cmark_iter_get_node(iter.get());
            if (node == root.get())
                continue;

            if (ev == CMARK_EVENT_ENTER)
            {
                auto entity = md_entity::try_parse(node, *stack.top());
                auto parent = stack.top();

                // push new scope
                if (is_container(entity->get_entity_type()))
                    stack.push(static_cast<md_container*>(entity.get()));

                // add entity to parent
                if (cmark_node_parent(node) == root.get())
                {
                    assert(parent == &comment.get_content());
                    direct_children.push_back(std::move(entity));
                }
                else
                    parent->add_entity(std::move(entity));
            }
            else if (ev == CMARK_EVENT_EXIT)
            {
                stack.pop();
            }
        }

        return direct_children;
    }

    std::string read_command(const parser& p, const md_entity& e)
    {
        if (e.get_entity_type() != md_entity::text_t)
            // must be a text
            return "";

        auto& text = static_cast<const md_text&>(e);
        auto  str  = text.get_string();
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

    void remove_command_string(md_text& text, const std::string& command)
    {
        // remove command + command character + whitespace
        auto  new_str = text.get_string() + command.size() + 1;
        while (std::isspace(*new_str))
            ++new_str;

        // need a copy, cmark can't handle it otherwise, https://github.com/jgm/cmark/issues/139
        text.set_string(std::string(new_str).c_str());
    }

    const char* read_argument(const md_text& text, const std::string& command)
    {
        auto string = text.get_string() + command.size() + 1;
        while (std::isspace(*string))
            ++string;

        return string;
    }

    const char* handle_command(comment& c, md_text& text, const std::string& command_str,
                               unsigned command)
    {
        if (is_section(command))
            throw comment_parse_error("Section type cannot appear in command-only paragraph",
                                      cmark_node_get_start_line(text.get_node()),
                                      cmark_node_get_start_column(text.get_node()));

        switch (make_command(command))
        {
        case command_type::exclude:
            c.set_excluded(true);
            break;
        case command_type::unique_name:
            c.set_unique_name_override(read_argument(text, command_str));
            break;
        case command_type::entity:
            return read_argument(text, command_str);
        case command_type::count:
        case command_type::invalid:
            throw comment_parse_error("Unknown command '" + command_str + "'",
                                      cmark_node_get_start_line(text.get_node()),
                                      cmark_node_get_start_column(text.get_node()));
        }

        return nullptr;
    }

    bool parse_command(const parser& p, comment& c, md_paragraph& paragraph, string& entity)
    {
        if (paragraph.begin()->get_entity_type() != md_entity::text_t)
            return true;

        auto& text        = static_cast<md_text&>(*paragraph.begin());
        auto  command_str = read_command(p, text);
        if (command_str.empty())
            // no command string, return as-is
            return true;

        auto command = p.get_comment_config().try_get_command(command_str);
        if (is_section(command))
        {
            // remove text string
            remove_command_string(text, command_str);
            // set section
            auto section = make_section(command);
            paragraph.set_section_type(section, p.get_output_config().get_section_name(section));
            return true;
        }

        auto entity_name = handle_command(c, text, command_str, command);
        for (auto iter = std::next(paragraph.begin()); iter != paragraph.end(); ++iter)
        {
            if (iter->get_entity_type() == md_entity::soft_break_t)
                // allow and skip soft breaks
                continue;

            command_str = read_command(p, *iter);
            if (command_str.empty())
                throw comment_parse_error("Normal markup will be ignored in command-only paragraph",
                                          cmark_node_get_start_line(text.get_node()),
                                          cmark_node_get_start_column(text.get_node()));
            command = p.get_comment_config().try_get_command(command_str);
            entity_name = handle_command(c, static_cast<md_text&>(*iter), command_str, command);
        }

        if (entity_name)
            entity = entity_name;

        // don't keep paragraph
        return false;
    }

    bool should_merge(md_paragraph* last_paragraph, section_type last_section,
                      section_type cur_section)
    {
        if (cur_section == section_type::details || cur_section == section_type::brief)
            return false;
        return last_paragraph && last_section == cur_section;
    }

    void merge(md_paragraph& last_paragraph, const md_paragraph& cur_paragraph)
    {
        for (auto& child : cur_paragraph)
            last_paragraph.add_entity(child.clone(last_paragraph));
    }

    string add_children(const parser& p, unsigned start_line, comment& comment,
                        std::vector<md_entity_ptr>& children)
    {
        string entity_name("");

        auto last_section   = section_type::brief;
        auto last_paragraph = static_cast<md_paragraph*>(nullptr);
        auto first          = true;
        for (auto& child : children)
            try
            {
                assert(child);
                if (child->get_entity_type() != md_entity::paragraph_t)
                    comment.get_content().add_entity(std::move(child));
                else
                {
                    auto& paragraph = static_cast<md_paragraph&>(*child);
                    if (first)
                    {
                        paragraph.set_section_type(section_type::brief, "");
                        first = false;
                    }
                    else
                        paragraph.set_section_type(section_type::details, "");

                    auto keep = parse_command(p, comment, paragraph, entity_name);
                    if (!keep)
                        continue;

                    if (should_merge(last_paragraph, last_section, paragraph.get_section_type()))
                        merge(*last_paragraph, paragraph);
                    else
                    {
                        last_paragraph = &paragraph;
                        last_section   = paragraph.get_section_type();
                        comment.get_content().add_entity(std::move(child));
                    }
                }
            }
            catch (comment_parse_error& error)
            {
                p.get_logger()->warn("when parsing comments ({}:{}): {}",
                                     start_line + error.get_line(), error.get_column(),
                                     error.what());
            }

        return entity_name;
    }
}

void standardese::parse_comments(const parser& p, const string& file_name,
                                 const std::string& source)
{
    auto raw_comments = detail::read_comments(source);
    for (auto& raw_comment : raw_comments)
    {
        comment c;

        auto document = parse_document(p, raw_comment.content);
        auto children = parse_children(c, document);
        auto entity_name =
            add_children(p, raw_comment.end_line - raw_comment.count_lines + 1, c, children);

        if (entity_name.empty())
            p.get_comment_registry().register_comment(comment_id(file_name, raw_comment.end_line),
                                                      std::move(c));
        else
            p.get_comment_registry().register_comment(comment_id(entity_name), std::move(c));
    }
}
