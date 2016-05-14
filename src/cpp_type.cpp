// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_type.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/parser.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

namespace
{
    cpp_type_ref parse_alias_target(cpp_cursor cur, const cpp_name &name)
    {
        auto type = clang_getTypedefDeclUnderlyingType(cur);

        if (clang_getCursorKind(cur) == CXCursor_TypeAliasDecl)
            return {type, detail::parse_alias_type_name(cur)};

        assert(clang_getCursorKind(cur) == CXCursor_TypedefDecl);

        auto str = detail::parse_typedef_type_name(cur, name);
        return {type, str};
    }
}

cpp_name cpp_type_ref::get_full_name() const
{
    return detail::parse_name(type_);
}

cpp_ptr<cpp_type_alias> cpp_type_alias::parse(translation_unit &tu, const cpp_name &scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_TypedefDecl
           || clang_getCursorKind(cur) == CXCursor_TypeAliasDecl);

    auto name = detail::parse_name(cur);
    auto target = parse_alias_target(cur, name);
    auto result = detail::make_ptr<cpp_type_alias>(scope, std::move(name), detail::parse_comment(cur),
                                                   clang_getCursorType(cur), target);

    tu.get_parser().register_type(*result);

    return result;
}