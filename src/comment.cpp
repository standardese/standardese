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
    if (!ptr)
        return back();

    if (ptr->get_entity_type() == md_entity::paragraph_t)
    {
        auto& par = static_cast<md_paragraph&>(*ptr);
        if (par.get_section_type() == section_type::brief)
        {
            // add all children to brief
            if (!get_brief().empty())
                get_brief().add_entity(md_soft_break::make(get_brief()));
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
    assert(!get_content().empty()); // always at least brief
    if (std::next(get_content().begin()) != get_content().end())
    {
        // we have more than a brief paragraph in the comment
        for (auto iter = std::next(get_content().begin()); iter != get_content().end(); ++iter)
            if (!is_container(iter->get_entity_type()))
                // consider not empty
                return false;
            else if (!static_cast<const md_container&>(*iter).empty())
                // non-empty container
                return false;
    }
    else
        // we only have brief
        return get_content().get_brief().empty();

    return true;
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
        assert(!clang_Cursor_isNull(cur) && !clang_isTranslationUnit(clang_getCursorKind(cur)));

        // we need the extent because we need the very first character of the cursor
        auto range    = clang_getCursorExtent(cur);
        auto location = clang_getRangeStart(range);

        CXString file;
        unsigned line;
        clang_getPresumedLocation(location, &file, &line, nullptr);

        return std::make_pair(file, line);
    }

    const cpp_entity& get_inline_parent(const cpp_entity& e)
    {
        if (e.get_entity_type() == cpp_entity::function_parameter_t)
            return *e.get_semantic_parent();
        else if (e.get_entity_type() == cpp_entity::template_type_parameter_t
                 || e.get_entity_type() == cpp_entity::non_type_template_parameter_t
                 || e.get_entity_type() == cpp_entity::template_template_parameter_t)
        {
            auto templ = e.get_semantic_parent();
            assert(templ && is_template(templ->get_entity_type()));
            return *templ;
        }
        else if (e.get_entity_type() == cpp_entity::base_class_t)
            return *e.get_semantic_parent();

        return e;
    }

    comment_id create_location_id(cpp_entity::type t, const std::pair<string, unsigned>& location,
                                  const cpp_name& name = "")
    {
        if (location.second == 1u)
            // entity is located at the first line of the file
            // only end-of-line comment possible
            return comment_id(location.first, 1u);
        else if (name.empty())
            return comment_id(location.first, location.second - 1);

        auto prefix = t == cpp_entity::base_class_t ? "::" : ".";
        return comment_id(location.first, location.second - 1,
                          detail::get_id(std::string(prefix) + name.c_str()));
    }

    template <class Entity>
    comment_id create_preprocessor_id(const Entity& e)
    {
        auto loc = std::make_pair(e.get_ast_parent().get_name(), e.get_line_number());
        return create_location_id(e.get_entity_type(), loc);
    }

    comment_id create_location_id(const cpp_entity& e)
    {
        if (e.get_entity_type() == cpp_entity::file_t)
            return comment_id(e.get_full_name());
        else if (e.get_entity_type() == cpp_entity::macro_definition_t)
            return create_preprocessor_id(static_cast<const cpp_macro_definition&>(e));
        else if (e.get_entity_type() == cpp_entity::inclusion_directive_t)
            return create_preprocessor_id(static_cast<const cpp_inclusion_directive&>(e));

        auto& parent    = get_inline_parent(e);
        auto  is_inline = &parent != &e;
        return create_location_id(e.get_entity_type(), get_location(parent.get_cursor()),
                                  is_inline ? e.get_name() : "");
    }

    bool inline_name_matches(const cpp_entity& e, const string& name)
    {
        if (e.get_entity_type() == cpp_entity::base_class_t)
        {
            if (std::strncmp(name.c_str(), "::", 2) != 0)
                return false;
            return std::strcmp(e.get_name().c_str(), name.c_str() + 2) == 0;
        }
        else
        {
            if (std::strncmp(name.c_str(), ".", 1) != 0)
                return false;
            return std::strcmp(e.get_name().c_str(), name.c_str() + 1) == 0;
        }
    }

    bool matches(const cpp_entity& e, const comment_id& id, cpp_cursor cur = {})
    {
        auto& inline_parent = get_inline_parent(e);

        if (id.is_name())
            return e.get_unique_name() == id.unique_name();
        else if (id.is_location() && e.get_entity_type() != cpp_entity::file_t)
        {
            if (&inline_parent != &e)
                // an entity that must have an inline location
                return false;

            auto e_id = create_location_id(e);
            if (id.file_name() != e_id.file_name())
                // wrong file
                return false;
            else if (id.line() > e_id.line() + 1)
                // next comment
                return false;

            return e_id.line() == id.line() || e_id.line() + 1 == id.line();
        }
        else if (id.is_inline_location())
        {
            if (&inline_parent == &e)
                // not an entity where an inline location makes sense
                return false;
            assert(clang_Cursor_isNull(cur));

            auto e_id = create_location_id(e);
            if (id.file_name() != e_id.file_name())
                // wrong file
                return false;
            else if (id.line() > e_id.line() + 1)
                // next comment
                return false;

            return (e_id.line() == id.line() || e_id.line() + 1 == id.line())
                   && inline_name_matches(e, id.inline_entity_name());
        }

        assert(e.get_entity_type() == cpp_entity::file_t);
        return false;
    }

    template <class Map>
    const comment* lookup_comment_location(Map& comments, const comment_id& id, const cpp_entity& e,
                                           cpp_cursor cur = {})
    {
        auto iter = comments.lower_bound(id);
        if (iter != comments.end())
        {
            // first try the next higher one, i.e. end of same line
            // then try the actual match
            ++iter;
            if (iter == comments.end() || !matches(e, iter->first, cur))
            {
                --iter;
                if (!matches(e, iter->first, cur))
                    return nullptr;
            }

            if (!iter->second.empty())
                return &iter->second;
            // this command is only used for commands, look for a remote comment
            auto& comment = iter->second;

            iter = comments.find(comment_id(comment.has_unique_name_override() ?
                                                comment.get_unique_name_override() :
                                                e.get_unique_name()));
            if (iter != comments.end())
                // add remote content
                comment.set_content(iter->second.get_content().clone());

            return &comment;
        }

        return nullptr;
    }
}

const comment* comment_registry::lookup_comment(const cpp_entity_registry& registry,
                                                const cpp_entity&          e) const
{
    std::unique_lock<std::mutex> lock(mutex_);

    // first look for comments at the location
    auto location = create_location_id(e);
    if (auto c = lookup_comment_location(comments_, location, e))
        return c;

    // then look for comments at alternative locations
    for (auto alternatives = registry.get_alternatives(e.get_cursor());
         alternatives.first != alternatives.second; ++alternatives.first)
    {
        auto& alternative = alternatives.first->second;
        auto  definition_location =
            create_location_id(e.get_entity_type(), get_location(alternative));
        if (auto c = lookup_comment_location(comments_, definition_location, e, alternative))
            return c;
    }

    // then for comments with the unique name
    auto id   = detail::get_id(e.get_unique_name().c_str());
    auto iter = comments_.find(comment_id(id));
    if (iter != comments_.end())
        return &iter->second;

    auto short_id = detail::get_short_id(id);
    if (id == short_id)
        return nullptr;
    iter = comments_.find(comment_id(short_id));
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

    md_node parse_document(const parser&, const string& raw_comment)
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
        cmark_parser_feed(parser.get(), raw_comment.c_str(), raw_comment.length());
        return cmark_parser_finish(parser.get());
    }

    struct comment_info
    {
        string               file_name, entity_name;
        standardese::comment comment;
        unsigned             begin_line, end_line;
        bool                 inline_comment;

        comment_info(string file_name, unsigned begin_line, unsigned end_line)
        : file_name(std::move(file_name)),
          entity_name(""),
          begin_line(begin_line),
          end_line(end_line),
          inline_comment(false)
        {
        }
    };

    void register_comment(const parser& p, comment_info& info)
    {
        auto& registry = p.get_comment_registry();
        if (info.inline_comment)
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

    std::string read_command(const parser& p, const md_entity& e)
    {
        if (e.get_entity_type() != md_entity::text_t)
            // must be a text
            return "";
        else if (e.get_parent().get_entity_type() != md_entity::paragraph_t)
            // parent must be a paragraph
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

    class container_stack
    {
    public:
        explicit container_stack(const parser& p, comment_info& info) : info_(&info), parser_(&p)
        {
        }

        void push(md_entity_ptr e)
        {
            if (!cur_inline_)
                stack_.emplace(std::move(e));
        }

        comment_info& push(comment_info info)
        {
            pop_info();
            cur_inline_.reset(new comment_info(std::move(info)));
            return *cur_inline_;
        }

        md_container& top() const
        {
            if (cur_inline_)
            {
                auto& comment = cur_inline_->comment.get_content();
                if (std::next(comment.begin()) == comment.end())
                {
                    // need a details paragraph
                    auto paragraph = md_paragraph::make(comment);
                    paragraph->set_section_type(section_type::details, "");
                    comment.add_entity(std::move(paragraph));
                }
                return static_cast<md_container&>(comment.back());
            }
            return stack_.empty() ? info_->comment.get_content() : *stack_.top().ptr;
        }

        comment_info& info()
        {
            return cur_inline_ ? *cur_inline_ : *info_;
        }

        void add_soft_break(md_entity_ptr e)
        {
            auto& container = top();
            if (!container.empty())
                // don't add as first element of paragraph
                container.add_entity(std::move(e));
        }

        md_paragraph& new_paragraph()
        {
            assert(top().get_entity_type() == md_entity::paragraph_t);
            if (!top().empty())
            {
                pop();
                push(md_paragraph::make(top()));
            }
            return static_cast<md_paragraph&>(top());
        }

        void pop()
        {
            assert(!stack_.empty());
            pop_info();
            // also need to pop() the parent paragraph it is in

            auto container = std::move(stack_.top());
            stack_.pop();

            if (!container.empty())
            {
                if (stack_.empty())
                    info_->comment.get_content().add_entity(
                        container.ptr->clone(info_->comment.get_content()));
                else
                    top().add_entity(std::move(container.ptr));
            }
        }

        void pop_info()
        {
            if (cur_inline_)
                register_comment(*parser_, *cur_inline_);
            cur_inline_.reset(nullptr);
        }

    private:
        struct container
        {
            md_ptr<md_container> ptr;

            container(md_entity_ptr ptr) : ptr(static_cast<md_container*>(ptr.release()))
            {
            }

            bool empty() const
            {
                if (ptr->empty())
                    return true;
                auto empty = true;
                for (auto& e : *ptr)
                    empty &= e.get_entity_type() == md_entity::soft_break_t;
                return empty;
            }
        };

        std::stack<container>         stack_;
        std::unique_ptr<comment_info> cur_inline_; // that's a lazy optional emulation, right there
        comment_info*                 info_;
        const parser*                 parser_;
    };

    std::size_t get_group_id(const char* name)
    {
        using hash = std::hash<std::string>;
        auto res   = hash{}(name);
        return res == 0u ? 19937u : res; // must not be 0
    }

    bool parse_command(const parser& p, container_stack& stack, md_entity_ptr text_entity)
    {
        assert(text_entity->get_entity_type() == md_entity::text_t);
        auto& text = static_cast<md_text&>(*text_entity);

        auto command_str = read_command(p, text);
        auto command     = p.get_comment_config().try_get_command(command_str);
        if (is_section(command))
        {
            auto  section   = make_section(command);
            auto& paragraph = stack.new_paragraph(); // start a new paragraph for the section
            paragraph.set_section_type(section, p.get_output_config().get_section_name(section));
            remove_command_string(text, command_str);       // remove the command of the string
            stack.top().add_entity(std::move(text_entity)); // add the text

            return true;
        }
        else if (is_command(command))
        {
            stack.new_paragraph().set_section_type(section_type::details,
                                                   ""); // terminate old paragraph
            switch (make_command(command))
            {
            case command_type::exclude:
                stack.info().comment.set_excluded(true);
                break;
            case command_type::unique_name:
                stack.info().comment.set_unique_name_override(read_argument(text, command_str));
                break;
            case command_type::synopsis:
                stack.info().comment.set_synopsis_override(read_argument(text, command_str));
                break;
            case command_type::group:
                stack.info().comment.add_to_member_group(
                    get_group_id(read_argument(text, command_str)));
                break;
            case command_type::entity:
                if (!stack.info().entity_name.empty())
                    throw comment_parse_error(fmt::format("Comment target already set to {}",
                                                          stack.info().entity_name.c_str()),
                                              text);
                stack.info().entity_name = read_argument(text, command_str);
                break;
            case command_type::file:
                if (!stack.info().entity_name.empty())
                    throw comment_parse_error(fmt::format("Comment target already set to {}",
                                                          stack.info().entity_name.c_str()),
                                              text);
                stack.info().entity_name = stack.info().file_name;
                break;
            case command_type::param:
            case command_type::tparam:
            case command_type::base:
            {
                // remove command
                remove_command_string(text, command_str);

                // read + remove param
                std::string param_name;
                for (auto ptr = text.get_string(); *ptr && !std::isspace(*ptr); ++ptr)
                    param_name += *ptr;
                remove_argument_string(text, param_name);

                // create comment info
                stack.pop_info();
                comment_info inline_info(stack.info().file_name, stack.info().begin_line,
                                         stack.info().end_line);
                auto separator = make_command(command) == command_type::base ? "::" : ".";
                if (!stack.info().entity_name.empty())
                {
                    // we already have a remote comment
                    // need to construct unique name, and register as name
                    inline_info.entity_name = std::string(stack.info().entity_name.c_str())
                                              + separator + std::move(param_name);
                }
                else
                {
                    // regular inline comment
                    inline_info.entity_name    = separator + std::move(param_name);
                    inline_info.inline_comment = true;
                }

                if (*text.get_string())
                    // non-empty text, add to brief
                    inline_info.comment.get_content().get_brief().add_entity(
                        std::move(text_entity));

                // add inline comment to stack
                stack.push(std::move(inline_info));
                break;
            }
            case command_type::invalid:
            case command_type::count:
                assert(false);
            }
        }
        else if (command_str.empty())
            // not a command, just add
            stack.top().add_entity(std::move(text_entity));
        else if (command == static_cast<unsigned int>(command_type::invalid))
            throw comment_parse_error(fmt::format("invalid command name '{}'", command_str), text);

        return false;
    }

    void parse_comment(const parser& p, comment_info& info, const md_node& root)
    {
        struct iter_deleter
        {
            void operator()(cmark_iter* iter) const STANDARDESE_NOEXCEPT
            {
                cmark_iter_free(iter);
            }
        };

        using md_iter = detail::wrapper<cmark_iter*, iter_deleter>;
        md_iter iter(cmark_iter_new(root.get()));

        container_stack stack(p, info);
        auto            first_paragraph = true, added_section = false;
        for (auto ev = CMARK_EVENT_NONE; (ev = cmark_iter_next(iter.get())) != CMARK_EVENT_DONE;)
        {
            auto node = cmark_iter_get_node(iter.get());
            if (node == root.get())
                continue;

            try
            {
                if (ev == CMARK_EVENT_ENTER)
                {
                    auto& parent = stack.top();
                    auto  entity = md_entity::try_parse(node, parent);
                    if (is_container(entity->get_entity_type()))
                    {
                        if (entity->get_entity_type() == md_entity::paragraph_t)
                        {
                            auto& paragraph = static_cast<md_paragraph&>(*entity);
                            if (paragraph.get_section_type() == section_type::invalid)
                                paragraph.set_section_type(first_paragraph ? section_type::brief :
                                                                             section_type::details,
                                                           "");
                            first_paragraph = false;
                        }
                        stack.push(std::move(entity));
                    }
                    else if (entity->get_entity_type() == md_entity::line_break_t
                             && stack.top().get_entity_type() == md_entity::paragraph_t)
                    {
                        // terminate current paragraph
                        auto& paragraph = stack.new_paragraph();
                        paragraph.set_section_type(first_paragraph ? section_type::brief :
                                                                     section_type::details,
                                                   "");
                        first_paragraph = false;
                    }
                    else if (entity->get_entity_type() == md_entity::text_t)
                        added_section |= parse_command(p, stack, std::move(entity));
                    else if (entity->get_entity_type() == md_entity::soft_break_t)
                        stack.add_soft_break(std::move(entity));
                    else
                        stack.top().add_entity(std::move(entity));
                }
                else if (ev == CMARK_EVENT_EXIT)
                {
                    stack.pop();
                    // reset first_paragraph if brief paragraph was empty
                    first_paragraph =
                        !added_section && stack.info().comment.get_content().get_brief().empty();
                }
                else
                    assert(false);
            }
            catch (comment_parse_error& error)
            {
                p.get_logger()->warn("when parsing comments ({}:{}): {}", error.get_line(),
                                     error.get_column(), error.what());
            }
        }

        register_comment(p, info);
    }
}

void standardese::parse_comments(const parser& p, const char* file_name, const std::string& source)
{
    auto raw_comments = detail::read_comments(source);
    for (auto& raw_comment : raw_comments)
    {
        comment_info info(file_name, raw_comment.end_line - raw_comment.count_lines + 1,
                          raw_comment.end_line);

        auto document = parse_document(p, raw_comment.content);
        parse_comment(p, info, document);
    }
}
