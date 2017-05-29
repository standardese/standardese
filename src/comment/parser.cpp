// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment/parser.hpp>

#include <cassert>
#include <cstring>

#include <cmark.h>

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

using namespace standardese;
using namespace standardese::comment;

parser::parser() : parser_(cmark_parser_new(CMARK_OPT_SMART))
{
}

parser::~parser()
{
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

    template <class Builder, typename T>
    auto add_child(int, Builder& b, std::unique_ptr<T> entity)
        -> decltype(b.add_child(std::move(entity)))
    {
        return b.add_child(std::move(entity));
    }

    template <class Builder, typename T>
    auto add_child(int, Builder& b, std::unique_ptr<T> entity)
        -> decltype(b.add_item(std::move(entity)))
    {
        return b.add_item(std::move(entity));
    }

    template <class Builder, typename T>
    void add_child(short, Builder&, std::unique_ptr<T>)
    {
        assert(!"unexpected child");
    }

    template <class Builder>
    void add_children(Builder& b, cmark_node* parent)
    {
        auto cur  = cmark_node_first_child(parent);
        auto last = cmark_node_last_child(parent);
        while (true)
        {
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
                throw translation_error(unsigned(cmark_node_get_start_line(cur)),
                                        unsigned(cmark_node_get_start_column(cur)),
                                        std::string("forbidden CommonMark node of type \"")
                                            + cmark_node_get_type_string(cur) + "\"");

            case CMARK_NODE_NONE:
            case CMARK_NODE_DOCUMENT:
                assert(!"invalid node type");
                break;
            }

            if (cur == last)
                break;
            cur = cmark_node_next(cur);
        }
    }
}

translated_ast standardese::comment::translate_ast(const ast_root& root)
{
    assert(cmark_node_get_type(root.get()) == CMARK_NODE_DOCUMENT);
    translated_ast result;

    // just assume details for now
    markup::details_section::builder builder;
    add_children(builder, root.get());
    result.sections.push_back(builder.finish());

    return result;
}
