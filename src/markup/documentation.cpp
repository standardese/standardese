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
                             std::unique_ptr<code_block> synopsis)
: block_entity(std::move(id)), heading_(std::move(heading)), synopsis_(std::move(synopsis))
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

entity_kind file_documentation::do_get_kind() const noexcept
{
    return entity_kind::file_documentation;
}

entity_kind entity_documentation::do_get_kind() const noexcept
{
    return entity_kind::entity_documentation;
}
