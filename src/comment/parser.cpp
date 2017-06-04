// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment/parser.hpp>

#include <cassert>
#include <cstring>
#include <type_traits>

#include <cmark.h>
#include <cmark_extension_api.h>

#if !defined(CMARK_NODE_TYPE_PRESENT)
#error "requires GFM cmark"
#endif

#include <standardese/markup/code_block.hpp>
#include <standardese/markup/link.hpp>
#include <standardese/markup/list.hpp>
#include <standardese/markup/heading.hpp>
#include <standardese/markup/paragraph.hpp>
#include <standardese/markup/quote.hpp>
#include <standardese/markup/thematic_break.hpp>
#include <standardese/markup/entity_kind.hpp>

#include "cmark_ext_command.hpp"

using namespace standardese;
using namespace standardese::comment;

parser::parser(comment::config c)
: config_(std::move(c)), parser_(cmark_parser_new(CMARK_OPT_SMART))
{
    auto ext = detail::create_command_extension(config_);
    cmark_parser_attach_syntax_extension(parser_, ext);
}

parser::~parser()
{
    auto cur = cmark_parser_get_syntax_extensions(parser_);
    while (cur)
    {
        cmark_syntax_extension_free(cmark_get_default_mem_allocator(),
                                    static_cast<cmark_syntax_extension*>(cur->data));
        cur = cur->next;
    }
    cmark_parser_free(parser_);
}

ast_root::~ast_root()
{
    if (root_)
        cmark_node_free(root_);
}

ast_root standardese::comment::read_ast(const parser& p, const std::string& comment)
{
    cmark_parser_feed(p.get(), comment.c_str(), comment.size());
    auto root = cmark_parser_finish(p.get());
    return ast_root(root);
}

namespace
{
    template <class Builder>
    void add_children(Builder& b, cmark_node* parent);

    [[noreturn]] void error(cmark_node* node, std::string msg)
    {
        throw translation_error(unsigned(cmark_node_get_start_line(node)),
                                unsigned(cmark_node_get_start_column(node)), std::move(msg));
    }

    std::unique_ptr<markup::block_quote> parse_block_quote(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_BLOCK_QUOTE);

        markup::block_quote::builder builder(markup::block_id{});
        add_children(builder, node);
        return builder.finish();
    }

    std::unique_ptr<markup::block_entity> parse_list(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_LIST);

        auto type = cmark_node_get_list_type(node);
        if (type == CMARK_BULLET_LIST)
        {
            markup::unordered_list::builder builder(markup::block_id{});
            add_children(builder, node);
            return builder.finish();
        }
        else if (type == CMARK_ORDERED_LIST)
        {
            markup::ordered_list::builder builder(markup::block_id{});
            add_children(builder, node);
            return builder.finish();
        }
        else
            assert(false);

        return nullptr;
    }

    std::unique_ptr<markup::list_item> parse_item(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_ITEM);

        markup::list_item::builder builder;
        add_children(builder, node);
        return builder.finish();
    }

    std::unique_ptr<markup::code_block> parse_code_block(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_CODE_BLOCK);
        return markup::code_block::build(markup::block_id{}, cmark_node_get_fence_info(node),
                                         cmark_node_get_literal(node));
    }

    std::unique_ptr<markup::paragraph> parse_paragraph(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_PARAGRAPH);

        markup::paragraph::builder builder;
        add_children(builder, node);
        return builder.finish();
    }

    std::unique_ptr<markup::block_entity> parse_heading(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_HEADING);

        auto level = cmark_node_get_heading_level(node);
        if (level == 1)
        {
            markup::heading::builder builder(markup::block_id{});
            add_children(builder, node);
            return builder.finish();
        }
        else
        {
            markup::subheading::builder builder(markup::block_id{});
            add_children(builder, node);
            return builder.finish();
        }
    }

    std::unique_ptr<markup::thematic_break> parse_thematic_break(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_THEMATIC_BREAK);
        (void)node;
        return markup::thematic_break::build();
    }

    std::unique_ptr<markup::text> parse_text(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_TEXT);
        return markup::text::build(cmark_node_get_literal(node));
    }

    std::unique_ptr<markup::soft_break> parse_softbreak(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_SOFTBREAK);
        return markup::soft_break::build();
    }

    std::unique_ptr<markup::hard_break> parse_linebreak(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_LINEBREAK);
        return markup::hard_break::build();
    }

    std::unique_ptr<markup::code> parse_code(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_CODE);
        return markup::code::build(cmark_node_get_literal(node));
    }

    std::unique_ptr<markup::emphasis> parse_emph(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_EMPH);
        markup::emphasis::builder builder;
        add_children(builder, node);
        return builder.finish();
    }

    std::unique_ptr<markup::strong_emphasis> parse_strong(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_STRONG);
        markup::strong_emphasis::builder builder;
        add_children(builder, node);
        return builder.finish();
    }

    markup::block_reference lookup_unique_name(const std::string& unique_name)
    {
        // TODO: do actual lookup
        return markup::block_reference(markup::block_id(unique_name));
    }

    std::unique_ptr<markup::external_link> parse_external_link(cmark_node* node, const char* title,
                                                               const char* url)
    {
        markup::external_link::builder builder(title, url);
        add_children(builder, node);
        return builder.finish();
    }

    std::unique_ptr<markup::link_base> parse_link(cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_LINK);

        auto title = cmark_node_get_title(node);
        auto url   = cmark_node_get_url(node);
        if (*url == '\0' && *title == '\0')
        {
            // both empty, unique name is child iff
            // child is a single text node
            auto child = cmark_node_first_child(node);
            if (child == nullptr || child != cmark_node_last_child(node)
                || cmark_node_get_type(child) != CMARK_NODE_TEXT)
                // not an entity link
                return parse_external_link(node, title, url);
            else
            {
                auto unique_name = cmark_node_get_literal(child);
                return markup::internal_link::builder(lookup_unique_name(unique_name)).finish();
            }
        }
        else if (*url == '\0')
        {
            // url is empty, unique name is title
            markup::internal_link::builder builder(lookup_unique_name(title));
            add_children(builder, node);
            return builder.finish();
        }
        else if (std::strncmp(url, "standardese://", std::strlen("standardese://")) == 0)
        {
            // standardese:// URL
            auto unique_name = std::string(url + std::strlen("standardese://"));
            if (!unique_name.empty() && unique_name.back() == '/')
                unique_name.pop_back();

            markup::internal_link::builder builder(title, lookup_unique_name(unique_name));
            add_children(builder, node);
            return builder.finish();
        }
        else
        {
            // regular link
            return parse_external_link(node, title, url);
        }

        assert(false);
        return nullptr;
    }

    std::unique_ptr<markup::phrasing_entity> parse_key(const char* key)
    {
        if (*key == '[')
        {
            // look for a closing ']', might have a link
            auto begin = ++key;
            while (*key && *key != ']')
                ++key;

            if (*key)
            {
                // found one
                auto                           unique_name = std::string(begin, key - begin);
                markup::internal_link::builder link(lookup_unique_name(unique_name));
                link.add_child(markup::text::build(std::move(unique_name)));
                return link.finish();
            }
        }

        // normal text
        return markup::text::build(key);
    }

    std::unique_ptr<markup::list_section> parse_list_section(cmark_node*& node)
    {
        auto type = detail::get_section_type(node);

        markup::unordered_list::builder builder{markup::block_id{}};
        do
        {
            auto key = detail::get_section_key(node);
            if (key)
            {
                // key-value item
                auto term = markup::term::build(parse_key(key));

                markup::description::builder description;
                add_children(description,
                             cmark_node_first_child(node)); // the children of the paragraph

                builder.add_item(markup::term_description_item::build(markup::block_id(),
                                                                      std::move(term),
                                                                      description.finish()));
            }
            else
            {
                // no key-value item, just add the normal paragraph
                auto paragraph = cmark_node_first_child(node);
                builder.add_item(markup::list_item::build(parse_paragraph(paragraph)));
            }

            node = cmark_node_next(node);
        } while (node && cmark_node_get_type(node) == detail::node_section()
                 && detail::get_section_type(node) == type);
        // went one too far
        node = cmark_node_previous(node);

        return markup::list_section::build(type, "", builder.finish()); // TODO
    }

    std::unique_ptr<markup::doc_section> parse_section(cmark_node*& node)
    {
        assert(cmark_node_get_type(node) == detail::node_section());
        switch (detail::get_section_type(node))
        {
        case section_type::brief:
        {
            assert(cmark_node_get_type(cmark_node_first_child(node)) == CMARK_NODE_PARAGRAPH);

            markup::brief_section::builder builder;
            add_children(builder, cmark_node_first_child(node));
            return builder.finish();
        }
        case section_type::details:
        {
            markup::details_section::builder builder;
            add_children(builder, node);
            return builder.finish();
        }

        case section_type::requires:
        case section_type::effects:
        case section_type::synchronization:
        case section_type::postconditions:
        case section_type::returns:
        case section_type::throws:
        case section_type::complexity:
        case section_type::remarks:
        case section_type::error_conditions:
        case section_type::notes:
        case section_type::see:
        {
            if (detail::get_section_key(node))
                return parse_list_section(node);
            else
            {
                auto paragraph = cmark_node_first_child(node);
                return markup::inline_section::build(detail::get_section_type(node), "", // TODO
                                                     parse_paragraph(paragraph));
            }
        }

        case section_type::count:
            assert(false);
            break;
        }

        return nullptr;
    }

    template <class Builder, typename T>
    auto add_child(int, Builder& b, std::unique_ptr<T> entity)
        -> decltype(void(b.add_child(std::move(entity))))
    {
        b.add_child(std::move(entity));
    }

    template <class Builder, typename T>
    auto add_child(int, Builder& b, std::unique_ptr<T> entity)
        -> decltype(void(b.add_item(std::move(entity))))
    {
        b.add_item(std::move(entity));
    }

    void add_child(int, translated_ast::builder& result, std::unique_ptr<markup::doc_section> ptr)
    {
        if (ptr->kind() == markup::entity_kind::brief_section)
        {
            auto brief = std::unique_ptr<markup::brief_section>(
                static_cast<markup::brief_section*>(ptr.release()));
            if (!result.brief(std::move(brief)))
                error(nullptr, "multiple brief sections for comment");
        }
        else
            result.add_section(std::move(ptr));
    }

    template <class Builder, typename T>
    void add_child(short, Builder&, std::unique_ptr<T>)
    {
        assert(!"unexpected child");
    }

    bool starts_with(const char* str, const char* target)
    {
        return std::strncmp(str, target, std::strlen(target)) == 0;
    }

    const char* get_single_arg(cmark_node* node, const char* cmd)
    {
        auto args = detail::get_command_arguments(node);
        for (auto ptr = args; *ptr; ++ptr)
            if (*ptr == ' ' || *ptr == '\t')
                error(node,
                      std::string("multiple arguments given for command '") + cmd
                          + "', but only one expected");
        return args;
    }

    void skip_ws(const char*& args)
    {
        while (*args && (*args == ' ' || *args == '\t'))
            ++args;
    }

    type_safe::optional<std::string> get_next_arg(const char*& args)
    {
        std::string result;
        while (*args && *args != ' ' && *args != '\t')
            result += *args++;
        skip_ws(args);
        return result.empty() ? type_safe::nullopt : type_safe::make_optional(std::move(result));
    }

    std::string get_next_required_arg(const char*& args, cmark_node* node, const char* command)
    {
        auto maybe_arg = get_next_arg(args);
        if (!maybe_arg)
            error(node, std::string("missing required argument command '") + command + "'");
        return maybe_arg.value();
    }

    exclude_mode parse_exclude_mode(cmark_node* node)
    {
        auto args = detail::get_command_arguments(node);
        if (starts_with(args, "return"))
            return exclude_mode::return_type;
        else if (starts_with(args, "target"))
            return exclude_mode::target;
        else if (*args)
            error(node, "invalid argument for exclude");
        else
            return exclude_mode::entity;
    }

    member_group parse_group(cmark_node* node)
    {
        auto args = detail::get_command_arguments(node);

        auto name    = get_next_required_arg(args, node, "group");
        auto heading = *args ? type_safe::make_optional(std::string(args)) : type_safe::nullopt;

        if (name.front() == '-')
        {
            // name starts with -, erase it, and don't consider it a section
            name.erase(name.begin());
            return member_group(std::move(name), std::move(heading), false);
        }
        else
            return member_group(std::move(name), std::move(heading), true);
    }

    void parse_command(translated_ast::builder& ast, cmark_node* node)
    {
        assert(cmark_node_get_type(node) == detail::node_command());
        auto command = detail::get_command_type(node);
        switch (command)
        {
        case command_type::exclude:
            if (!ast.exclude(parse_exclude_mode(node)))
                error(node, "multiple exclude commands for entity");
            break;

        case command_type::unique_name:
            if (!ast.unique_name(get_single_arg(node, "unique name")))
                error(node, "multiple unique name commands for entity");
            break;
        case command_type::synopsis:
            if (!ast.synopsis(detail::get_command_arguments(node)))
                error(node, "multiple synopsis commands for entity");
            break;

        case command_type::group:
            if (!ast.group(parse_group(node)))
                error(node, "multiple group commands for entity");
            break;
        case command_type::module:
            if (!ast.module(get_single_arg(node, "module")))
                error(node, "multiple module commands for entity");
            break;
        case command_type::output_section:
            if (!ast.output_section(detail::get_command_arguments(node)))
                error(node, "multiple output section commands for entity");
            break;

        case command_type::entity:
            if (!ast.entity(remote_entity(get_single_arg(node, "entity"))))
                error(node, "multiple file/entity commands for entity");
            break;
        case command_type::file:
            if (*detail::get_command_arguments(node))
                error(node, "arguments given to file command but none required");
            if (!ast.entity(current_file{}))
                error(node, "multiple file/entity commands for entity");
            break;

        case command_type::param:
            break;
        case command_type::tparam:
            break;
        case command_type::base:
            break;

        case command_type::count:
        case command_type::invalid:
            error(node, "invalid command");
        }
    }

    template <typename T>
    void parse_command(const T&, cmark_node*)
    {
        assert(!"unexpected command");
    }

    template <class Builder>
    void add_children(Builder& b, cmark_node* parent)
    {
        for (auto cur = cmark_node_first_child(parent); cur; cur = cmark_node_next(cur))
        {
            if (cmark_node_get_type(cur) == detail::node_section())
                add_child(0, b, parse_section(cur));
            else if (cmark_node_get_type(cur) == detail::node_command())
                parse_command(b, cur);
            else
                switch (cmark_node_get_type(cur))
                {
                case CMARK_NODE_BLOCK_QUOTE:
                    add_child(0, b, parse_block_quote(cur));
                    break;
                case CMARK_NODE_LIST:
                    add_child(0, b, parse_list(cur));
                    break;
                case CMARK_NODE_ITEM:
                    add_child(0, b, parse_item(cur));
                    break;
                case CMARK_NODE_CODE_BLOCK:
                    add_child(0, b, parse_code_block(cur));
                    break;
                case CMARK_NODE_PARAGRAPH:
                    add_child(0, b, parse_paragraph(cur));
                    break;
                case CMARK_NODE_HEADING:
                    add_child(0, b, parse_heading(cur));
                    break;
                case CMARK_NODE_THEMATIC_BREAK:
                    add_child(0, b, parse_thematic_break(cur));
                    break;

                case CMARK_NODE_TEXT:
                    add_child(0, b, parse_text(cur));
                    break;
                case CMARK_NODE_SOFTBREAK:
                    add_child(0, b, parse_softbreak(cur));
                    break;
                case CMARK_NODE_LINEBREAK:
                    add_child(0, b, parse_linebreak(cur));
                    break;
                case CMARK_NODE_CODE:
                    add_child(0, b, parse_code(cur));
                    break;
                case CMARK_NODE_EMPH:
                    add_child(0, b, parse_emph(cur));
                    break;
                case CMARK_NODE_STRONG:
                    add_child(0, b, parse_strong(cur));
                    break;
                case CMARK_NODE_LINK:
                    add_child(0, b, parse_link(cur));
                    break;

                case CMARK_NODE_HTML_BLOCK:
                case CMARK_NODE_HTML_INLINE:
                case CMARK_NODE_CUSTOM_BLOCK:
                case CMARK_NODE_CUSTOM_INLINE:
                case CMARK_NODE_IMAGE:
                    error(cur,
                          std::string("forbidden CommonMark node of type \"")
                              + cmark_node_get_type_string(cur) + "\"");

                case CMARK_NODE_NONE:
                case CMARK_NODE_DOCUMENT:
                    assert(!"invalid node type");
                    break;
                }
        }
    }
}

translated_ast standardese::comment::translate_ast(const parser&, const ast_root& root)
{
    assert(cmark_node_get_type(root.get()) == CMARK_NODE_DOCUMENT);

    translated_ast::builder builder;
    add_children(builder, root.get());

    return builder.finish();
}
