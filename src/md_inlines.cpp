// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/md_inlines.hpp>

#include <cassert>
#include <cmark.h>

#include <spdlog/fmt/fmt.h>

#include <standardese/error.hpp>

using namespace standardese;

md_ptr<md_text> md_text::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_TEXT);
    return detail::make_md_ptr<md_text>(cur, parent);
}

md_ptr<md_text> md_text::make(const md_entity& parent, const char* text)
{
    auto node = cmark_node_new(CMARK_NODE_TEXT);
    if (!node)
        throw cmark_error("md_text::make");
    if (!cmark_node_set_literal(node, text))
        throw cmark_error("md_text::make: cmark_node_set_literal");
    return detail::make_md_ptr<md_text>(node, parent);
}

md_entity_ptr md_text::do_clone(const md_entity* parent) const
{
    assert(parent);
    return std::move(make(*parent, get_string()));
}

md_ptr<md_soft_break> md_soft_break::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_SOFTBREAK);
    return detail::make_md_ptr<md_soft_break>(cur, parent);
}

md_ptr<md_soft_break> md_soft_break::make(const md_entity& parent)
{
    auto node = cmark_node_new(CMARK_NODE_SOFTBREAK);
    if (!node)
        throw cmark_error("md_soft_break::make");
    return detail::make_md_ptr<md_soft_break>(node, parent);
}

md_entity_ptr md_soft_break::do_clone(const md_entity* parent) const
{
    assert(parent);
    return std::move(make(*parent));
}

md_ptr<md_line_break> md_line_break::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_LINEBREAK);
    return detail::make_md_ptr<md_line_break>(cur, parent);
}

md_ptr<md_line_break> md_line_break::make(const md_entity& parent)
{
    auto node = cmark_node_new(CMARK_NODE_LINEBREAK);
    if (!node)
        throw cmark_error("md_line_break::make");
    return detail::make_md_ptr<md_line_break>(node, parent);
}

md_entity_ptr md_line_break::do_clone(const md_entity* parent) const
{
    assert(parent);
    return std::move(make(*parent));
}

md_ptr<md_code> md_code::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_CODE);
    return detail::make_md_ptr<md_code>(cur, parent);
}

md_ptr<md_code> md_code::make(const md_entity& parent, const char* code)
{
    auto node = cmark_node_new(CMARK_NODE_CODE);
    if (!node)
        throw cmark_error("md_code::make");
    if (!cmark_node_set_literal(node, code))
        throw cmark_error("md_code::make: cmark_node_set_literal");
    return detail::make_md_ptr<md_code>(node, parent);
}

md_entity_ptr md_code::do_clone(const md_entity* parent) const
{
    assert(parent);
    return std::move(make(*parent, get_string()));
}

md_ptr<md_emphasis> md_emphasis::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_EMPH);
    return detail::make_md_ptr<md_emphasis>(cur, parent);
}

md_ptr<md_emphasis> md_emphasis::make(const md_entity& parent)
{
    auto node = cmark_node_new(CMARK_NODE_EMPH);
    if (!node)
        throw cmark_error("md_emphasis::make");
    return detail::make_md_ptr<md_emphasis>(node, parent);
}

md_ptr<md_emphasis> md_emphasis::make(const md_entity& parent, const char* str)
{
    auto emph = make(parent);

    auto text = md_text::make(*emph, str);
    emph->add_entity(std::move(text));

    return emph;
}

md_entity_ptr md_emphasis::do_clone(const md_entity* parent) const
{
    assert(parent);

    auto result = make(*parent);
    for (auto& child : *this)
        result->add_entity(child.clone(*result));

    return std::move(result);
}

md_ptr<md_strong> md_strong::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_STRONG);
    return detail::make_md_ptr<md_strong>(cur, parent);
}

md_ptr<md_strong> md_strong::make(const md_entity& parent)
{
    auto node = cmark_node_new(CMARK_NODE_STRONG);
    if (!node)
        throw cmark_error("md_strong::make");
    return detail::make_md_ptr<md_strong>(node, parent);
}

md_ptr<md_strong> md_strong::make(const md_entity& parent, const char* str)
{
    auto strong = make(parent);

    auto text = md_text::make(*strong, str);
    strong->add_entity(std::move(text));

    return strong;
}

md_entity_ptr md_strong::do_clone(const md_entity* parent) const
{
    assert(parent);

    auto result = make(*parent);
    for (auto& child : *this)
        result->add_entity(child.clone(*result));

    return std::move(result);
}

md_ptr<md_link> md_link::parse(cmark_node* cur, const md_entity& parent)
{
    assert(cmark_node_get_type(cur) == CMARK_NODE_LINK);
    return detail::make_md_ptr<md_link>(cur, parent);
}

md_ptr<md_link> md_link::make(const md_entity& parent, const char* destination, const char* title)
{
    auto node = cmark_node_new(CMARK_NODE_LINK);
    if (!node)
        throw cmark_error("md_link::make");
    if (!cmark_node_set_url(node, destination))
        throw cmark_error("md_link::make: cmark_node_set_url");
    if (!cmark_node_set_title(node, title))
        throw cmark_error("md_link::make: cmark_node_set_title");
    return detail::make_md_ptr<md_link>(node, parent);
}

const char* md_link::get_title() const STANDARDESE_NOEXCEPT
{
    return cmark_node_get_title(get_node());
}

const char* md_link::get_destination() const STANDARDESE_NOEXCEPT
{
    return cmark_node_get_url(get_node());
}

void md_link::set_destination(const char* dest)
{
    auto result = cmark_node_set_url(get_node(), dest);
    if (!result)
        throw cmark_error("md_link::set_destination");
}

md_entity_ptr md_link::do_clone(const md_entity* parent) const
{
    assert(parent);

    auto result = make(*parent, get_destination(), get_title());
    for (auto& child : *this)
        result->add_entity(child.clone(*result));

    return std::move(result);
}

namespace
{
    std::string make_id(const char* id)
    {
        return fmt::format("<a id=\"{}\"></a>", id);
    }
}

md_ptr<md_anchor> md_anchor::make(const md_entity& parent, const char* id)
{
    auto node = cmark_node_new(CMARK_NODE_HTML_INLINE);
    if (!node)
        throw cmark_error("md_anchor::make");
    if (!cmark_node_set_literal(node, make_id(id).c_str()))
        throw cmark_error("md_anchor::make: cmark_node_set_literal");
    return detail::make_md_ptr<md_anchor>(node, parent);
}

std::string md_anchor::get_id() const
{
    static const auto offset_beginning = std::strlen("<a id=\"");
    static const auto offset_end       = std::strlen("\"></a>");

    auto str = cmark_node_get_literal(get_node());

    std::string result(str + offset_beginning);
    result.erase(result.begin() + (result.size() - offset_end), result.end());

    return result;
}

void md_anchor::set_id(const char* id)
{
    if (!cmark_node_set_literal(get_node(), make_id(id).c_str()))
        throw cmark_error("md_anchor::set_literal");
}

md_entity_ptr md_anchor::do_clone(const md_entity* parent) const
{
    assert(parent);
    return make(*parent, get_id().c_str());
}
