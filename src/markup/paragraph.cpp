// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/paragraph.hpp>

#include <standardese/markup/entity_kind.hpp>

using namespace standardese::markup;

entity_kind paragraph::do_get_kind() const noexcept
{
    return entity_kind::paragraph;
}

void paragraph::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> paragraph::do_clone() const
{
    builder b(id());
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<phrasing_entity>(child.clone()));
    return b.finish();
}
