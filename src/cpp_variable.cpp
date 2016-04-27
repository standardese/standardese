// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_variable.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/search_token.hpp>

using namespace standardese;

#include <iostream>

namespace
{
    cpp_type_ref parse_variable_type(cpp_cursor cur, const cpp_name &name, std::string &initializer)
    {
        assert(clang_getCursorKind(cur) == CXCursor_VarDecl);

        auto type = clang_getCursorType(cur);
        auto type_name = detail::parse_variable_type_name(cur, name, initializer);

        return {type, std::move(type_name)};
    }

    cpp_linkage convert_linkage(CXType type, CX_StorageClass storage) STANDARDESE_NOEXCEPT
    {
        switch (storage)
        {
            case CX_SC_None:
                return clang_isConstQualifiedType(type) && !clang_isVolatileQualifiedType(type)
                      ? cpp_internal_linkage : cpp_no_linkage;
            case CX_SC_Extern:
                return cpp_external_linkage;
            case CX_SC_Static:
                return cpp_internal_linkage;
            case CX_SC_Invalid:
                return cpp_no_linkage;
            default:
                assert(false);
        }
    }

    bool is_variable_thread_local(cpp_cursor cur, const cpp_name &name)
    {
        return detail::has_prefix_token(cur, "thread_local", name.c_str());
    }
}

cpp_ptr<cpp_variable> cpp_variable::parse(cpp_name scope, cpp_cursor cur)
{
    auto name = detail::parse_name(cur);
    std::string initializer;
    auto type = parse_variable_type(cur, name, initializer);
    auto linkage = convert_linkage(type.get_type(), clang_Cursor_getStorageClass(cur));
    auto is_thread_local = is_variable_thread_local(cur, name);

    return detail::make_ptr<cpp_variable>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                          std::move(type), std::move(initializer), linkage, is_thread_local);
}
