// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/documentation.hpp>

#include <standardese/markup/code_block.hpp>
#include <standardese/markup/entity_kind.hpp>
#include <standardese/markup/heading.hpp>

using namespace standardese::markup;

documentation::~documentation() noexcept = default;

documentation::documentation(block_id id, std::unique_ptr<markup::heading> heading,
                             std::unique_ptr<code_block>      synopsis,
                             type_safe::optional<std::string> module)
: block_entity(std::move(id)),
  module_(std::move(module)),
  heading_(std::move(heading)),
  synopsis_(std::move(synopsis))
{
}

type_safe::optional_ref<const standardese::markup::brief_section> documentation::brief_section()
    const noexcept
{
    for (auto& section : sections_)
        if (section->kind() == entity_kind::brief_section)
            return type_safe::ref(static_cast<const markup::brief_section&>(*section));
    return nullptr;
}

type_safe::optional_ref<const standardese::markup::details_section> documentation::details_section()
    const noexcept
{
    for (auto& section : sections_)
        if (section->kind() == entity_kind::details_section)
            return type_safe::ref(static_cast<const markup::details_section&>(*section));
    return nullptr;
}

void documentation::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    cb(mem, heading());
    cb(mem, synopsis());

    if (auto brief = brief_section())
        cb(mem, brief.value());
    if (auto details = details_section())
        cb(mem, details.value());
    for (auto& section : doc_sections())
        cb(mem, section);

    for (auto& child : *this)
        cb(mem, child);
}

file_documentation::builder::builder(block_id id, std::unique_ptr<standardese::markup::heading> h,
                                     std::unique_ptr<code_block>      synopsis,
                                     type_safe::optional<std::string> module)
: documentation_builder(std::unique_ptr<file_documentation>(
      new file_documentation(std::move(id), std::move(h), std::move(synopsis), std::move(module))))
{
}

entity_kind file_documentation::do_get_kind() const noexcept
{
    return entity_kind::file_documentation;
}

entity_documentation::builder::builder(block_id id, std::unique_ptr<standardese::markup::heading> h,
                                       std::unique_ptr<code_block>      synopsis,
                                       type_safe::optional<std::string> module)
: documentation_builder(std::unique_ptr<entity_documentation>(
      new entity_documentation(std::move(id), std::move(h), std::move(synopsis),
                               std::move(module))))
{
}

entity_kind entity_documentation::do_get_kind() const noexcept
{
    return entity_kind::entity_documentation;
}
