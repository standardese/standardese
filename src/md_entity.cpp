// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/md_entity.hpp>

#include <cassert>
#include <cmark.h>
#include <spdlog/details/format.h>

#include <standardese/error.hpp>
#include <standardese/md_blocks.hpp>
#include <standardese/md_inlines.hpp>

using namespace standardese;

md_entity_ptr md_entity::try_parse(comment& c, cmark_node* cur, const md_entity& parent)
{
    switch (cmark_node_get_type(cur))
    {
#define STANDARDESE_DETAIL_HANDLE(value, name)                                                     \
    case CMARK_NODE_##value:                                                                       \
        return md_##name::parse(c, cur, parent);

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
    cmark_node_unlink(node_);
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
    cmark_node_set_literal(get_node(), str);
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

void md_container::add_entity(md_entity_ptr entity)
{
    if (cmark_node_parent(entity->get_node()) != entity->get_parent().get_node())
    {
        // synthesized node, need to add
        cmark_node_unlink(entity->get_node());
        cmark_node_append_child(entity->get_parent().get_node(), entity->get_node());
    }

    md_entity_container::add_entity(std::move(entity));
}