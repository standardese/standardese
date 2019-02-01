// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/code_block.hpp>

#include <standardese/markup/entity_kind.hpp>

using namespace standardese::markup;

entity_kind code_block::keyword_tag::kind() noexcept
{
    return entity_kind::code_block_keyword;
}

entity_kind code_block::identifier_tag::kind() noexcept
{
    return entity_kind::code_block_identifier;
}

entity_kind code_block::string_literal_tag::kind() noexcept
{
    return entity_kind::code_block_string_literal;
}

entity_kind code_block::int_literal_tag::kind() noexcept
{
    return entity_kind::code_block_int_literal;
}

entity_kind code_block::float_literal_tag::kind() noexcept
{
    return entity_kind::code_block_float_literal;
}

entity_kind code_block::punctuation_tag::kind() noexcept
{
    return entity_kind::code_block_punctuation;
}

entity_kind code_block::preprocessor_tag::kind() noexcept
{
    return entity_kind::code_block_preprocessor;
}

entity_kind code_block::do_get_kind() const noexcept
{
    return entity_kind::code_block;
}

void code_block::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> code_block::do_clone() const
{
    builder b(id(), language());
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<phrasing_entity>(child.clone()));
    return b.finish();
}
