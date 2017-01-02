// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/md_entity.hpp>

#include <cassert>
#include <cmark.h>
#include <spdlog/fmt/fmt.h>

#include <standardese/error.hpp>
#include <standardese/md_blocks.hpp>
#include <standardese/md_inlines.hpp>

using namespace standardese;

md_entity_ptr md_entity::try_parse(cmark_node* cur, const md_entity& parent)
{
    switch (cmark_node_get_type(cur))
    {
#define STANDARDESE_DETAIL_HANDLE(value, name)                                                     \
    case CMARK_NODE_##value:                                                                       \
        return md_##name::parse(cur, parent);

        STANDARDESE_DETAIL_HANDLE(BLOCK_QUOTE, block_quote)
        STANDARDESE_DETAIL_HANDLE(LIST, list)
        STANDARDESE_DETAIL_HANDLE(ITEM, list_item)
        STANDARDESE_DETAIL_HANDLE(CODE_BLOCK, code_block)
        STANDARDESE_DETAIL_HANDLE(PARAGRAPH, paragraph)
        STANDARDESE_DETAIL_HANDLE(HEADING, heading)
        STANDARDESE_DETAIL_HANDLE(THEMATIC_BREAK, thematic_break)

        STANDARDESE_DETAIL_HANDLE(TEXT, text)
        STANDARDESE_DETAIL_HANDLE(SOFTBREAK, soft_break)
        STANDARDESE_DETAIL_HANDLE(LINEBREAK, line_break)
        STANDARDESE_DETAIL_HANDLE(CODE, code)
        STANDARDESE_DETAIL_HANDLE(EMPH, emphasis)
        STANDARDESE_DETAIL_HANDLE(STRONG, strong)
        STANDARDESE_DETAIL_HANDLE(LINK, link)

#undef STANDARDESE_DETAIL_HANDLE

    case CMARK_NODE_HTML_INLINE:
    {
        // return text node instead
        auto text = cmark_node_get_literal(cur);
        return md_text::make(parent, text);
    }

    default:
        break;
    }

    auto line   = cmark_node_get_start_line(cur);
    auto column = cmark_node_get_start_column(cur);
    throw comment_parse_error(fmt::format("Forbidden CommonMark node of type '{}'",
                                          cmark_node_get_type_string(cur)),
                              line, column);
}

md_entity::~md_entity() STANDARDESE_NOEXCEPT
{
    if (cmark_node_parent(node_) == nullptr)
        cmark_node_free(node_);
}

const char* md_leave::get_string() const STANDARDESE_NOEXCEPT
{
    auto string = cmark_node_get_literal(get_node());
    return string ? string : "";
}

md_leave::md_leave(md_entity::type t, cmark_node* node,
                   const md_entity& parent) STANDARDESE_NOEXCEPT : md_entity(t, node, parent)
{
    assert(is_leave(t));
}

void md_leave::set_string(const char* str)
{
    auto res = cmark_node_set_literal(get_node(), str);
    if (!res)
        throw cmark_error("md_leave::set_string()");
}

md_container::md_container(md_entity::type t, cmark_node* node,
                           const md_entity& parent) STANDARDESE_NOEXCEPT
    : md_entity(t, node, parent)
{
    assert(is_container(t));
}

md_container::md_container(md_entity::type t, cmark_node* node) STANDARDESE_NOEXCEPT
    : md_entity(t, node)
{
    assert(is_container(t));
}

md_entity& md_container::add_entity(md_entity_ptr entity)
{
    if (!entity)
        return back();

    auto back = get_last();
    if (back && back->get_entity_type() == md_entity::text_t
        && entity->get_entity_type() == md_entity::text_t)
    {
        // need to merge adjacent text nodes
        // can happen because inline HTML is converted to text
        auto& old_text = static_cast<md_text&>(*back);
        auto& new_text = static_cast<md_text&>(*entity);
        old_text.set_string((std::string(old_text.get_string()) + new_text.get_string()).c_str());
        return old_text;
    }

    // just add normal
    entity->parent_ = this;
    if (cmark_node_parent(entity->get_node()) != get_node())
    {
        // synthesized node, need to add
        cmark_node_unlink(entity->get_node());
        auto res = cmark_node_append_child(get_node(), entity->get_node());
        if (!res)
            throw cmark_error("md_container::add_entity");
    }

    auto& ref = *entity;
    md_entity_container::add_entity(std::move(entity));
    return ref;
}