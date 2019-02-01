// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/documentation.hpp>

#include <standardese/markup/code_block.hpp>
#include <standardese/markup/entity_kind.hpp>
#include <standardese/markup/heading.hpp>

using namespace standardese::markup;

documentation_header documentation_header::clone() const
{
    return documentation_header(markup::clone(heading()), module());
}

type_safe::optional_ref<const standardese::markup::brief_section> documentation_entity::
    brief_section() const noexcept
{
    for (auto& section : sections_)
        if (section->kind() == entity_kind::brief_section)
            return type_safe::ref(static_cast<const markup::brief_section&>(*section));
    return nullptr;
}

type_safe::optional_ref<const standardese::markup::details_section> documentation_entity::
    details_section() const noexcept
{
    for (auto& section : sections_)
        if (section->kind() == entity_kind::details_section)
            return type_safe::ref(static_cast<const markup::details_section&>(*section));
    return nullptr;
}

entity_kind entity_documentation::do_get_kind() const noexcept
{
    return entity_kind::entity_documentation;
}

void entity_documentation::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    if (header())
        cb(mem, header().value().heading());
    if (synopsis())
        cb(mem, synopsis().value());

    for (auto& sec : doc_sections())
        cb(mem, sec);

    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> entity_documentation::do_clone() const
{
    builder b(entity_, id(),
              header() ? type_safe::make_optional(header().value().clone()) : type_safe::nullopt,
              synopsis() ? markup::clone(synopsis().value()) : nullptr);
    for (auto& sec : doc_sections())
        b.add_section_impl(detail::unchecked_downcast<doc_section>(sec.clone()));
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<entity_documentation>(child.clone()));
    return b.finish();
}

entity_kind file_documentation::do_get_kind() const noexcept
{
    return entity_kind::file_documentation;
}

void file_documentation::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    if (header())
        cb(mem, header().value().heading());
    if (synopsis())
        cb(mem, synopsis().value());

    for (auto& sec : doc_sections())
        cb(mem, sec);

    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> file_documentation::do_clone() const
{
    builder b(file_, id(),
              header() ? type_safe::make_optional(header().value().clone()) : type_safe::nullopt,
              synopsis() ? markup::clone(synopsis().value()) : nullptr);
    for (auto& sec : doc_sections())
        b.add_section_impl(detail::unchecked_downcast<doc_section>(sec.clone()));
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<entity_documentation>(child.clone()));
    return b.finish();
}
