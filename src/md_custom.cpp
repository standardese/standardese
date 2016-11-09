// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/md_custom.hpp>

#include <cassert>
#include <cmark.h>

#include <standardese/md_inlines.hpp>

using namespace standardese;

md_ptr<md_section> md_section::make(const md_entity& parent, const std::string& section_text)
{
    return detail::make_md_ptr<md_section>(parent, section_text);
}

const char* md_section::get_section_text() const STANDARDESE_NOEXCEPT
{
    assert(begin()->get_entity_type() == md_entity::emphasis_t);
    auto& emph = static_cast<const md_emphasis&>(*begin());
    assert(emph.begin()->get_entity_type() == md_entity::text_t);
    return static_cast<const md_text&>(*emph.begin()).get_string();
}

void md_section::set_section_text(const std::string& text)
{
    assert(begin()->get_entity_type() == md_entity::emphasis_t);
    auto& emph = static_cast<md_emphasis&>(*begin());
    assert(emph.begin()->get_entity_type() == md_entity::text_t);
    static_cast<md_text&>(*emph.begin()).set_string(text.c_str());

    assert(std::next(begin())->get_entity_type() == md_entity::text_t);
    static_cast<md_text&>(*std::next(begin())).set_string(text.empty() ? "" : ": ");
}

md_entity_ptr md_section::do_clone(const md_entity* parent) const
{
    assert(parent);
    return md_section::make(*parent, get_section_text());
}

md_section::md_section(const md_entity& parent, const std::string& section_text)
: md_container(get_entity_type(), cmark_node_new(CMARK_NODE_CUSTOM_INLINE), parent)
{
    auto emphasis = md_emphasis::make(*this, section_text.c_str());
    add_entity(std::move(emphasis));

    auto buffer_text = md_text::make(*this, section_text.empty() ? "" : ": ");
    add_entity(std::move(buffer_text));
}

md_ptr<md_document> md_document::make(std::string name)
{
    return detail::make_md_ptr<md_document>(cmark_node_new(CMARK_NODE_DOCUMENT), std::move(name));
}

md_entity_ptr md_document::do_clone(const md_entity* parent) const
{
    assert(!parent);
    (void)parent;

    auto result = make(name_);
    for (auto& child : *this)
        result->add_entity(child.clone(*result));
    return std::move(result);
}
