// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/link.hpp>

#include <standardese/markup/entity_kind.hpp>

using namespace standardese::markup;

void link_base::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

entity_kind external_link::do_get_kind() const noexcept
{
    return entity_kind::external_link;
}

entity_kind internal_link::do_get_kind() const noexcept
{
    return entity_kind::internal_link;
}
