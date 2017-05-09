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

entity_kind term::do_get_kind() const noexcept
{
    return entity_kind::term;
}

entity_kind description::do_get_kind() const noexcept
{
    return entity_kind::description;
}

entity_kind term_description_item::do_get_kind() const noexcept
{
    return entity_kind::term_description_item;
}

entity_kind unordered_list::do_get_kind() const noexcept
{
    return entity_kind::unordered_list;
}

entity_kind ordered_list::do_get_kind() const noexcept
{
    return entity_kind::ordered_list;
}
