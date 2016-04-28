// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_function.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>

using namespace standardese;

namespace
{
    cpp_type_ref parse_parameter_type(cpp_cursor cur, const cpp_name &name,
                                      std::string &default_value)
    {
        assert(clang_getCursorKind(cur) == CXCursor_ParmDecl);

        auto type = clang_getCursorType(cur);
        auto type_name = detail::parse_variable_type_name(cur, name, default_value);

        return {type, std::move(type_name)};
    }
}

cpp_ptr<cpp_function_parameter> cpp_function_parameter::parse(cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_ParmDecl);

    auto name = detail::parse_name(cur);
    std::string default_value;
    auto type = parse_parameter_type(cur, name, default_value);

    return detail::make_ptr<cpp_function_parameter>(std::move(name), detail::parse_comment(cur),
                                                    std::move(type), std::move(default_value));
}

namespace
{
    cpp_type_ref parse_function_info(cpp_cursor cur, const cpp_name &name,
                                     cpp_function_flags &flags, std::string &noexcept_expr)
    {
        auto type = clang_getCursorResultType(cur);

        int f = 0, d = 0;
        auto type_name = detail::parse_function_info(cur, name, f, noexcept_expr);

        if (clang_isFunctionTypeVariadic(clang_getCursorType(cur)))
            f |= cpp_variadic_fnc;

        flags = cpp_function_flags(f);

        return {type, std::move(type_name)};
    }

    void parse_parameters(cpp_function_base *base, cpp_cursor cur)
    {
        detail::visit_children(cur, [&](CXCursor cur, CXCursor)
        {
            if (clang_getCursorKind(cur) == CXCursor_ParmDecl)
                base->add_parameter(cpp_function_parameter::parse(cur));
            return CXChildVisit_Continue;
        });
    }
}

cpp_ptr<cpp_function> cpp_function::parse(cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_FunctionDecl);

    auto name = detail::parse_name(cur);
    cpp_function_flags flags;
    cpp_function_definition def = cpp_function_definition_normal; // libclang doesn't support it yet here
    std::string noexcept_expr;
    auto return_type = parse_function_info(cur, name, flags, noexcept_expr);

    auto result = detail::make_ptr<cpp_function>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                                 std::move(return_type), std::move(noexcept_expr),
                                                 flags, def);

    parse_parameters(result.get(), cur);

    return result;
}