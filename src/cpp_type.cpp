// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_type.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/search_token.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

namespace
{
    cpp_name parse_alias_target(cpp_cursor cur, const cpp_name &name)
    {
        if (clang_getCursorKind(cur) == CXCursor_TypeAliasDecl)
            return detail::cat_tokens_after(cur, "=");
        auto str = detail::cat_tokens_after(cur, "typedef");

        auto pos = str.find(name);
        str.erase(pos, name.size());

        return str;
    }
}

cpp_ptr<cpp_type_alias> cpp_type_alias::parse(const parser &p, const cpp_name &scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_TypedefDecl
           || clang_getCursorKind(cur) == CXCursor_TypeAliasDecl);

    cpp_ptr<cpp_type_alias> result(new cpp_type_alias(scope, detail::parse_name(cur), detail::parse_comment(cur)));

    auto target = clang_getTypedefDeclUnderlyingType(cur);
    string spelling(clang_getTypeSpelling(target));

    result->target_ = parse_alias_target(cur, result->get_name());
    result->unique_ = spelling.get();

    p.register_type(*result);

    return result;
}
