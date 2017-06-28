// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/list.hpp>

#include <standardese/markup/entity_kind.hpp>

using namespace standardese::markup;

entity_kind list_item::do_get_kind() const noexcept
{
    return entity_kind::list_item;
}

void list_item::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> list_item::do_clone() const
{
    builder b(id());
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<block_entity>(child.clone()));
    return b.finish();
}

entity_kind term::do_get_kind() const noexcept
{
    return entity_kind::term;
}

void term::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> term::do_clone() const
{
    builder b;
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<phrasing_entity>(child.clone()));
    return b.finish();
}

entity_kind description::do_get_kind() const noexcept
{
    return entity_kind::description;
}

void description::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> description::do_clone() const
{
    builder b;
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<phrasing_entity>(child.clone()));
    return b.finish();
}

entity_kind term_description_item::do_get_kind() const noexcept
{
    return entity_kind::term_description_item;
}

void term_description_item::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    cb(mem, term());
    cb(mem, description());
}

std::unique_ptr<entity> term_description_item::do_clone() const
{
    return build(id(), detail::unchecked_downcast<markup::term>(term().clone()),
                 detail::unchecked_downcast<markup::description>(description().clone()));
}

entity_kind unordered_list::do_get_kind() const noexcept
{
    return entity_kind::unordered_list;
}

void unordered_list::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> unordered_list::do_clone() const
{
    builder b(id());
    for (auto& child : *this)
        b.add_item(detail::unchecked_downcast<list_item_base>(child.clone()));
    return b.finish();
}

entity_kind ordered_list::do_get_kind() const noexcept
{
    return entity_kind::ordered_list;
}

void ordered_list::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> ordered_list::do_clone() const
{
    builder b(id());
    for (auto& child : *this)
        b.add_item(detail::unchecked_downcast<list_item_base>(child.clone()));
    return b.finish();
}
