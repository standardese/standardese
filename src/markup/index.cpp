// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/index.hpp>

#include <standardese/markup/code_block.hpp>
#include <standardese/markup/entity_kind.hpp>
#include <standardese/markup/heading.hpp>

using namespace standardese::markup;

entity_kind entity_index_item::do_get_kind() const noexcept
{
    return entity_kind::entity_index_item;
}

void entity_index_item::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    cb(mem, entity());
    if (brief())
        cb(mem, brief().value());
}

std::unique_ptr<entity> entity_index_item::do_clone() const
{
    return build(id(), detail::unchecked_downcast<term>(entity().clone()),
                 brief() ? detail::unchecked_downcast<description>(brief().value().clone()) :
                           nullptr);
}

entity_kind file_index::do_get_kind() const noexcept
{
    return entity_kind::file_index;
}

void file_index::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    cb(mem, heading());
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> file_index::do_clone() const
{
    builder b(detail::unchecked_downcast<markup::heading>(heading().clone()));
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<entity_index_item>(child.clone()));
    return b.finish();
}

entity_kind namespace_documentation::do_get_kind() const noexcept
{
    return entity_kind::namespace_documentation;
}

void namespace_documentation::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    if (header())
        cb(mem, header().value().heading());
    for (auto& sec : doc_sections())
        cb(mem, sec);

    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> namespace_documentation::do_clone() const
{
    builder b(ns_, id(),
              header() ? type_safe::make_optional(header().value().clone()) : type_safe::nullopt);
    for (auto& sec : doc_sections())
        b.add_section_impl(detail::unchecked_downcast<doc_section>(sec.clone()));
    for (auto& child : *this)
        b.container_builder::add_child(detail::unchecked_downcast<block_entity>(child.clone()));
    return b.finish();
}

entity_kind entity_index::do_get_kind() const noexcept
{
    return entity_kind::entity_index;
}

void entity_index::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    cb(mem, heading());
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> entity_index::do_clone() const
{
    builder b(detail::unchecked_downcast<markup::heading>(heading().clone()));
    for (auto& child : *this)
        b.container_builder::add_child(
            detail::unchecked_downcast<markup::block_entity>(child.clone()));
    return b.finish();
}

entity_kind module_documentation::do_get_kind() const noexcept
{
    return entity_kind::module_documentation;
}

void module_documentation::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    if (header())
        cb(mem, header().value().heading());
    for (auto& sec : doc_sections())
        cb(mem, sec);

    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> module_documentation::do_clone() const
{
    builder b(id(),
              header() ? type_safe::make_optional(header().value().clone()) : type_safe::nullopt);
    for (auto& sec : doc_sections())
        b.add_section_impl(detail::unchecked_downcast<doc_section>(sec.clone()));
    for (auto& child : *this)
        b.container_builder::add_child(
            detail::unchecked_downcast<entity_index_item>(child.clone()));
    return b.finish();
}

entity_kind module_index::do_get_kind() const noexcept
{
    return entity_kind::module_index;
}

void module_index::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    cb(mem, heading());
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> module_index::do_clone() const
{
    builder b(detail::unchecked_downcast<markup::heading>(heading().clone()));
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<module_documentation>(child.clone()));
    return b.finish();
}
