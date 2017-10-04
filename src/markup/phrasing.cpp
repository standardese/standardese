// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/phrasing.hpp>

#include <standardese/markup/entity_kind.hpp>

using namespace standardese::markup;

entity_kind text::do_get_kind() const noexcept
{
    return entity_kind::text;
}

void text::do_visit(detail::visitor_callback_t, void*) const
{
}

std::unique_ptr<entity> text::do_clone() const
{
    return build(text_);
}

entity_kind emphasis::do_get_kind() const noexcept
{
    return entity_kind::emphasis;
}

void emphasis::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> emphasis::do_clone() const
{
    builder b;
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<phrasing_entity>(child.clone()));
    return b.finish();
}

entity_kind strong_emphasis::do_get_kind() const noexcept
{
    return entity_kind::strong_emphasis;
}

void strong_emphasis::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> strong_emphasis::do_clone() const
{
    builder b;
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<phrasing_entity>(child.clone()));
    return b.finish();
}

entity_kind code::do_get_kind() const noexcept
{
    return entity_kind::code;
}

void code::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> code::do_clone() const
{
    builder b;
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<phrasing_entity>(child.clone()));
    return b.finish();
}

entity_kind soft_break::do_get_kind() const noexcept
{
    return entity_kind::soft_break;
}

void soft_break::do_visit(detail::visitor_callback_t, void*) const
{
}

std::unique_ptr<entity> soft_break::do_clone() const
{
    return build();
}

entity_kind hard_break::do_get_kind() const noexcept
{
    return entity_kind::hard_break;
}

void hard_break::do_visit(detail::visitor_callback_t, void*) const
{
}

std::unique_ptr<entity> hard_break::do_clone() const
{
    return build();
}
