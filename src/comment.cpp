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
#include <standardese/index.hpp>
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

bool comment::empty() const STANDARDESE_NOEXCEPT
{
    if (get_content().empty())
        return true;
    else if (std::next(get_content().begin()) != get_content().end())
        return false;
    else if (get_content().get_brief().empty())
        return true;
    return false;
}

bool comment_registry::register_comment(comment_id id, comment c) const
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto result = comments_.insert(std::make_pair(std::move(id), std::move(c)));
    return result.second;
}

namespace
{
    std::pair<string, unsigned> get_location(const cpp_cursor& cur)
    {
        assert(!clang_isTranslationUnit(clang_getCursorKind(cur)));
        auto location = clang_getCursorLocation(cur);

        CXFile   file;
        unsigned line;
        clang_getSpellingLocation(location, &file, &line, nullptr, nullptr);
        assert(file);

        return std::make_pair(clang_getFileName(file), line);
    }

    const cpp_entity& get_inline_parent(const cpp_entity& e)
    {
        if (e.get_entity_type() == cpp_entity::function_parameter_t)
        {
            assert(e.has_parent());

            auto& func = e.get_parent();
            assert(is_function_like(func.get_entity_type()) && func.has_parent());
            if (is_function_template(func.get_parent().get_entity_type()))
                return func.get_parent();
            return func;
        }

        return e;
    }

    comment_id create_location_id(const cpp_entity& e)
    {
        if (e.get_entity_type() == cpp_entity::file_t)
            return comment_id(e.get_full_name());

        auto& parent   = get_inline_parent(e);
        auto  location = get_location(parent.get_cursor());
        if (location.second == 1u)
            // entity is located at the first line of the file
            // no comment possible
            return comment_id("", 1u);

        auto is_inline = &parent != &e;
        if (is_inline)
            return comment_id(location.first, location.second - 1, e.get_name());
        return comment_id(location.first, location.second - 1);
    }

    bool matches(const cpp_entity& e, const comment_id& id)
    {
        if (id.is_name())
            return e.get_unique_name() == id.unique_name();
        else if (id.is_location() && e.get_entity_type() != cpp_entity::file_t)
        {
            auto location = get_location(e.get_cursor());
            return location.second - id.line() <= 1u && location.first == id.file_name();
        }
        else if (id.is_inline_location())
        {
            auto& inline_parent = get_inline_parent(e);
            if (&inline_parent == &e)
                // not an entity where an inline location makes sense
                return false;

            auto location = get_location(inline_parent.get_cursor());
            return location.second - id.line() <= 1u && location.first == id.file_name()
                   && e.get_name() == id.inline_entity_name();
        }

        assert(e.get_entity_type() == cpp_entity::file_t);
        return false;
    }

    template <class Map>
    const comment* lookup_comment_location(Map& comments, const comment_id& id, const cpp_entity& e)
    {
        auto iter = comments.lower_bound(id);
        if (iter != comments.end() && matches(e, iter->first))
        {
            if (!iter->second.empty())
                return &iter->second;
            // this command is only used for commands, look for a remote comment
            auto& comment = iter->second;

            iter = comments.find(comment_id(comment.has_unique_name_override() ?
                                                comment.get_unique_name_override() :
                                                e.get_unique_name()));
            if (iter != comments.end())
                // merge remote contents
                for (auto& child : iter->second.get_content())
                    comment.get_content().add_entity(child.clone(comment.get_content()));

            return &comment;
        }

        return nullptr;
    }
}

const comment* comment_registry::lookup_comment(const cpp_entity& e) const
{
    std::unique_lock<std::mutex> lock(mutex_);

    // first look for comments at the location
    auto location = create_location_id(e);
    if (auto c = lookup_comment_location(comments_, location, e))
        return c;

    // then for comments with the unique name
    auto id   = detail::get_id(e.get_unique_name().c_str());
    auto iter = comments_.find(comment_id(id));
    if (iter != comments_.end())
        return &iter->second;

    auto short_id = detail::get_short_id(id);
    iter          = comments_.find(comment_id(short_id));
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

    void remove_argument_string(md_text& text, const std::string& arg, unsigned offset = 0u)
    {
        // remove arg + offset + whitespace
        auto new_str = text.get_string() + arg.size() + offset;
        while (std::isspace(*new_str))
            ++new_str;

        // need a copy, cmark can't handle it otherwise, https://github.com/jgm/cmark/issues/139
        text.set_string(std::string(new_str).c_str());
    }

    void remove_command_string(md_text& text, const std::string& command)
    {
        remove_argument_string(text, command, 1u);
    }

    const char* read_argument(const md_text& text, const std::string& command)
    {
        auto string = text.get_string() + command.size() + 1;
        while (std::isspace(*string))
            ++string;

        return string;
    }

    struct comment_info
    {
        string               file_name, entity_name;
        standardese::comment comment;
        unsigned             begin_line, end_line;

        comment_info(string file_name, unsigned begin_line, unsigned end_line)
        : file_name(std::move(file_name)),
          entity_name(""),
          begin_line(begin_line),
          end_line(end_line)
        {
        }
    };

    void register_comment(const parser& p, comment_info& info, bool is_inline = false)
    {
        auto& registry = p.get_comment_registry();
        if (is_inline)
            registry.register_comment(comment_id(info.file_name, info.end_line,
                                                 detail::get_id(info.entity_name.c_str())),
                                      std::move(info.comment));
        else if (!info.entity_name.empty())
            registry.register_comment(comment_id(detail::get_id(info.entity_name.c_str())),
                                      std::move(info.comment));
        else
            registry.register_comment(comment_id(info.file_name, info.end_line),
                                      std::move(info.comment));
    }

    bool parse_command(const parser& p, comment_info& info, md_paragraph& paragraph);

    bool handle_command(const parser& p, comment_info& info, md_paragraph& paragraph, md_text& text,
                        const std::string& command_str, unsigned command)
    {
        if (is_section(command))
            throw comment_parse_error("Section type cannot appear in command-only paragraph",
                                      cmark_node_get_start_line(text.get_node()),
                                      cmark_node_get_start_column(text.get_node()));

        switch (make_command(command))
        {
        case command_type::exclude:
            info.comment.set_excluded(true);
            break;
        case command_type::unique_name:
            info.comment.set_unique_name_override(read_argument(text, command_str));
            break;
        case command_type::entity:
            if (!info.entity_name.empty())
                throw comment_parse_error(fmt::format("Comment target already set to {}",
                                                      info.entity_name.c_str()),
                                          cmark_node_get_start_line(text.get_node()),
                                          cmark_node_get_start_column(text.get_node()));
            info.entity_name = read_argument(text, command_str);
            break;
        case command_type::file:
            if (!info.entity_name.empty())
                throw comment_parse_error(fmt::format("Comment target already set to {}",
                                                      info.entity_name.c_str()),
                                          cmark_node_get_start_line(text.get_node()),
                                          cmark_node_get_start_column(text.get_node()));
            info.entity_name = info.file_name;
            break;
        case command_type::param:
        {
            // remove command
            remove_command_string(text, command_str);

            // read + remove param
            std::string param_name;
            for (auto ptr = text.get_string(); *ptr && !std::isspace(*ptr); ++ptr)
                param_name += *ptr;
            remove_argument_string(text, param_name);

            comment_info inline_info(info.file_name, info.begin_line, info.end_line);

            auto keep               = parse_command(p, inline_info, paragraph);
            if (keep)
                inline_info.comment.get_content().add_entity(
                    paragraph.clone(inline_info.comment.get_content()));

            if (!info.entity_name.empty())
            {
                // we already have a remote comment
                // need to construct unique name, and register as name
                inline_info.entity_name =
                    std::string(info.entity_name.c_str()) + "::" + std::move(param_name);
                register_comment(p, inline_info);
            }
            else
            {
                // regular inline comment
                inline_info.entity_name = std::move(param_name);
                register_comment(p, inline_info, true);
            }
        }
            return false;
        case command_type::count:
        case command_type::invalid:
            throw comment_parse_error("Unknown command '" + command_str + "'",
                                      info.begin_line + cmark_node_get_start_line(text.get_node()),
                                      cmark_node_get_start_column(text.get_node()));
        }

        return true;
    }

    void handle_commands(const parser& p, comment_info& info, md_paragraph& paragraph,
                         md_text& text, std::string command_str, unsigned command)
    {
        if (!handle_command(p, info, paragraph, text, command_str, command))
            return;
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
            if (!handle_command(p, info, paragraph, static_cast<md_text&>(*iter), command_str,
                                command))
                return;
        }
    }

    md_text* get_text(md_paragraph& paragraph)
    {
        for (auto iter = paragraph.begin(); iter != paragraph.end(); ++iter)
        {
            if (iter->get_entity_type() == md_entity::soft_break_t)
                // skip softbreaks
                continue;
            else if (iter->get_entity_type() != md_entity::text_t)
                return nullptr;
            auto& text = static_cast<md_text&>(*iter);
            if (!*text.get_string())
                // empty text
                continue;
            return &text;
        }

        return nullptr;
    }

    bool parse_command(const parser& p, comment_info& info, md_paragraph& paragraph)
    {
        auto text = get_text(paragraph);
        if (!text)
            return true;

        auto command_str = read_command(p, *text);
        if (command_str.empty())
            // no command string, return as-is
            return true;

        auto command = p.get_comment_config().try_get_command(command_str);
        if (is_section(command))
        {
            // remove text string
            remove_command_string(*text, command_str);
            // set section
            auto section = make_section(command);
            paragraph.set_section_type(section, p.get_output_config().get_section_name(section));
            return true;
        }

        handle_commands(p, info, paragraph, *text, command_str, command);
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

    void finish_parsing(const parser& p, comment_info& info, std::vector<md_entity_ptr>& children)
    {
        auto last_section   = section_type::brief;
        auto last_paragraph = static_cast<md_paragraph*>(nullptr);
        auto first          = true;
        for (auto& child : children)
            try
            {
                assert(child);
                if (child->get_entity_type() != md_entity::paragraph_t)
                    info.comment.get_content().add_entity(std::move(child));
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

                    auto keep = parse_command(p, info, paragraph);
                    if (!keep)
                        continue;

                    if (should_merge(last_paragraph, last_section, paragraph.get_section_type()))
                        merge(*last_paragraph, paragraph);
                    else
                    {
                        last_paragraph = &paragraph;
                        last_section   = paragraph.get_section_type();
                        info.comment.get_content().add_entity(std::move(child));
                    }
                }
            }
            catch (comment_parse_error& error)
            {
                p.get_logger()->warn("when parsing comments ({}:{}): {}", error.get_line(),
                                     error.get_column(), error.what());
            }

        register_comment(p, info);
    }
}

void standardese::parse_comments(const parser& p, const string& file_name,
                                 const std::string& source)
{
    auto raw_comments = detail::read_comments(source);
    for (auto& raw_comment : raw_comments)
    {
        comment_info info(file_name, raw_comment.end_line - raw_comment.count_lines + 1,
                          raw_comment.end_line);

        auto document = parse_document(p, raw_comment.content);
        auto children = parse_children(info.comment, document);
        finish_parsing(p, info, children);
    }
}
