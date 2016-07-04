// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/md_blocks.hpp>

#include <cassert>
#include <cmark.h>

#include <standardese/md_inlines.hpp>

using namespace standardese;

md_ptr<md_block_quote> md_block_quote::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_BLOCK_QUOTE);
    return detail::make_md_ptr<md_block_quote>(cur, parent);
}

md_ptr<md_block_quote> md_block_quote::make(const md_entity& parent)
{
    auto node = cmark_node_new(CMARK_NODE_BLOCK_QUOTE);
    return detail::make_md_ptr<md_block_quote>(node, parent);
}

md_ptr<md_list> md_list::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_LIST);
    return detail::make_md_ptr<md_list>(cur, parent);
}

md_ptr<md_list> md_list::make(const md_entity& parent, md_list_type type, md_list_delimiter delim,
                              int start, bool is_tight)
{
    auto node = cmark_node_new(CMARK_NODE_LIST);

    switch (type)
    {
    case md_list_type::bullet:
        cmark_node_set_list_type(node, CMARK_BULLET_LIST);
        break;
    case md_list_type::ordered:
        cmark_node_set_list_type(node, CMARK_ORDERED_LIST);
        break;
    }

    switch (delim)
    {
    case md_list_delimiter::none:
        cmark_node_set_list_delim(node, CMARK_NO_DELIM);
        break;
    case md_list_delimiter::parenthesis:
        cmark_node_set_list_delim(node, CMARK_PAREN_DELIM);
        break;
    case md_list_delimiter::period:
        cmark_node_set_list_delim(node, CMARK_PERIOD_DELIM);
        break;
    }

    cmark_node_set_list_start(node, start);
    cmark_node_set_list_tight(node, is_tight);

    return detail::make_md_ptr<md_list>(node, parent);
}

md_list_type md_list::get_list_type() const STANDARDESE_NOEXCEPT
{
    switch (cmark_node_get_list_type(get_node()))
    {
    case CMARK_BULLET_LIST:
        return md_list_type::bullet;
    case CMARK_ORDERED_LIST:
        return md_list_type::ordered;
    case CMARK_NO_LIST:
        break;
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

md_ptr<md_list_item> md_list_item::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_ITEM);
    return detail::make_md_ptr<md_list_item>(cur, parent);
}

md_ptr<md_list_item> md_list_item::make(const md_entity& parent)
{
    auto node = cmark_node_new(CMARK_NODE_ITEM);
    return detail::make_md_ptr<md_list_item>(node, parent);
}

md_ptr<md_code_block> md_code_block::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_CODE_BLOCK);
    return detail::make_md_ptr<md_code_block>(cur, parent);
}

md_ptr<standardese::md_code_block> md_code_block::make(const md_entity& parent, const char* code,
                                                       const char* fence)
{
    auto node = cmark_node_new(CMARK_NODE_CODE_BLOCK);
    cmark_node_set_literal(node, code);
    cmark_node_set_fence_info(node, fence);
    return detail::make_md_ptr<md_code_block>(node, parent);
}

const char* md_code_block::get_fence_info() const STANDARDESE_NOEXCEPT
{
    return cmark_node_get_fence_info(get_node());
}

md_ptr<md_paragraph> md_paragraph::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_PARAGRAPH);
    return detail::make_md_ptr<md_paragraph>(cur, parent);
}

md_ptr<md_paragraph> md_paragraph::make(const md_entity& parent)
{
    auto node = cmark_node_new(CMARK_NODE_PARAGRAPH);
    return detail::make_md_ptr<md_paragraph>(node, parent);
}

void md_paragraph::set_section_type(section_type t, const std::string& name)
{
    section_type_ = t;

    if (name.empty())
    {
        cmark_node_unlink(section_node_->get_node());
    }
    else
    {
        auto& emph = static_cast<md_emphasis&>(*section_node_);
        assert(emph.begin()->get_entity_type() == md_entity::text_t);

        // set section text
        static_cast<md_text&>(*emph.begin()).set_string((name + ':').c_str());

        // add leading whitespace to first paragraph text
        assert(begin()->get_entity_type() == md_entity::text_t);
        auto& text = static_cast<md_text&>(*begin());
        text.set_string((' ' + std::string(text.get_string())).c_str());

        // add the section node as child on the cmark api
        auto res = cmark_node_prepend_child(get_node(), section_node_->get_node());
        assert(res);
    }
}

md_paragraph::md_paragraph(cmark_node* node, const md_entity& parent)
: md_container(get_entity_type(), node, parent),
  section_node_(md_emphasis::make(*this, "")),
  section_type_(section_type::invalid)
{
}

md_ptr<md_heading> md_heading::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_HEADING);
    return detail::make_md_ptr<md_heading>(cur, parent);
}

md_ptr<md_heading> md_heading::make(const md_entity& parent, int level)
{
    auto node = cmark_node_new(CMARK_NODE_HEADING);
    cmark_node_set_heading_level(node, level);
    return detail::make_md_ptr<md_heading>(node, parent);
}

int md_heading::get_level() const STANDARDESE_NOEXCEPT
{
    return cmark_node_get_heading_level(get_node());
}

md_ptr<md_thematic_break> md_thematic_break::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_THEMATIC_BREAK);
    return detail::make_md_ptr<md_thematic_break>(cur, parent);
}

md_ptr<md_thematic_break> md_thematic_break::make(const md_entity& parent)
{
    auto node = cmark_node_new(CMARK_NODE_THEMATIC_BREAK);
    return detail::make_md_ptr<md_thematic_break>(node, parent);
}