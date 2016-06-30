// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/md_blocks.hpp>

#include <cassert>
#include <cmark.h>

using namespace standardese;

md_ptr<md_block_quote> md_block_quote::parse(comment&, cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_BLOCK_QUOTE);
    return detail::make_md_ptr<md_block_quote>(cur, parent);
}

md_ptr<md_list> md_list::parse(comment&, cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_LIST);
    return detail::make_md_ptr<md_list>(cur, parent);
}

md_list_type md_list::get_list_type() const STANDARDESE_NOEXCEPT
{
    switch (cmark_node_get_list_type(get_node()))
    {
    case CMARK_BULLET_LIST:
        return md_list_type::bullet;
    case CMARK_ORDERED_LIST:
        return md_list_type::ordered;
    }
    assert(false);
    return md_list_type::bullet;
}

md_list_delimiter md_list::get_delimiter() const STANDARDESE_NOEXCEPT
{
    switch (cmark_node_get_list_delim(get_node()))
    {
    case CMARK_NO_DELIM:
        return md_list_delimiter::none;
    case CMARK_PAREN_DELIM:
        return md_list_delimiter::parenthesis;
    case CMARK_PERIOD_DELIM:
        return md_list_delimiter::period;
    }
    assert(false);
    return md_list_delimiter::none;
}

int md_list::get_start() const STANDARDESE_NOEXCEPT
{
    return cmark_node_get_list_start(get_node());
}

bool md_list::is_tight() const STANDARDESE_NOEXCEPT
{
    return cmark_node_get_list_tight(get_node()) == 1;
}

md_ptr<md_list_item> md_list_item::parse(comment&, cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_ITEM);
    return detail::make_md_ptr<md_list_item>(cur, parent);
}

md_ptr<md_code_block> md_code_block::parse(comment&, cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_CODE_BLOCK);
    return detail::make_md_ptr<md_code_block>(cur, parent);
}

const char* md_code_block::get_fence_info() const STANDARDESE_NOEXCEPT
{
    return cmark_node_get_fence_info(get_node());
}

md_ptr<md_paragraph> md_paragraph::parse(comment&, cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_PARAGRAPH);
    return detail::make_md_ptr<md_paragraph>(cur, parent);
}

md_ptr<md_heading> md_heading::parse(comment&, cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_HEADING);
    return detail::make_md_ptr<md_heading>(cur, parent);
}

md_ptr<md_thematic_break> md_thematic_break::parse(comment&, cmark_node* cur,
                                                   const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_THEMATIC_BREAK);
    return detail::make_md_ptr<md_thematic_break>(cur, parent);
}
