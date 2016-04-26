// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_namespace.hpp>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/search_token.hpp>
#include <standardese/cpp_cursor.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

namespace
{
    bool is_inline_namespace(CXCursor cur, const cpp_name &name)
    {
        return detail::has_prefix_token(cur, "inline", name.c_str());
    }
}

cpp_namespace::parser::parser(const cpp_name &scope, cpp_cursor cur)
: ns_(new cpp_namespace(scope, detail::parse_name(cur), detail::parse_comment(cur)))
{
    if (is_inline_namespace(cur, ns_->get_name()))
        ns_->inline_ = true;

}

cpp_entity_ptr cpp_namespace::parser::finish(const standardese::parser &par)
{
    par.register_namespace(*ns_);
    return std::move(ns_);
}

