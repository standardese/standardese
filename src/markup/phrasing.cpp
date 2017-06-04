// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/phrasing.hpp>

#include <standardese/markup/entity_kind.hpp>

using namespace standardese::markup;

entity_kind text::do_get_kind() const noexcept
{
    return entity_kind::text;
}

entity_kind emphasis::do_get_kind() const noexcept
{
    return entity_kind::emphasis;
}

entity_kind strong_emphasis::do_get_kind() const noexcept
{
    return entity_kind::strong_emphasis;
}

entity_kind code::do_get_kind() const noexcept
{
    return entity_kind::code;
}

entity_kind soft_break::do_get_kind() const noexcept
{
    return entity_kind::soft_break;
}

entity_kind hard_break::do_get_kind() const noexcept
{
    return entity_kind::hard_break;
}
