// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/visitor.hpp>

#include <standardese/markup/entity.hpp>

using namespace standardese::markup;

void detail::call_visit(const entity& e, detail::visitor_callback_t cb, void* mem)
{
    e.do_visit(cb, mem);
}
