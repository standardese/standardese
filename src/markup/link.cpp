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

std::unique_ptr<entity> external_link::do_clone() const
{
    builder b(title(), url());
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<phrasing_entity>(child.clone()));
    return b.finish();
}

entity_kind internal_link::do_get_kind() const noexcept
{
    return entity_kind::internal_link;
}

std::unique_ptr<entity> internal_link::do_clone() const
{
    if (unresolved_destination())
    {
        builder b(title(), unresolved_destination().value());
        for (auto& child : *this)
            b.add_child(detail::unchecked_downcast<phrasing_entity>(child.clone()));
        return b.finish();
    }
    else
    {
        builder b(title(), destination().value());
        for (auto& child : *this)
            b.add_child(detail::unchecked_downcast<phrasing_entity>(child.clone()));
        return b.finish();
    }
}
