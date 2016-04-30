// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_variable.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/search_token.hpp>

using namespace standardese;

namespace
{
    cpp_type_ref parse_variable_type(cpp_cursor cur, const cpp_name &name, std::string &initializer)
    {
        assert(clang_getCursorKind(cur) == CXCursor_VarDecl
             || clang_getCursorKind(cur) == CXCursor_FieldDecl);

        auto type = clang_getCursorType(cur);
        auto type_name = detail::parse_variable_type_name(cur, name, initializer);

        return {type, std::move(type_name)};
    }

    bool is_variable_static_class(cpp_cursor cur) STANDARDESE_NOEXCEPT
    {
        auto parent = clang_getCursorSemanticParent(cur);
        auto kind = clang_getCursorKind(parent);
        return kind == CXCursor_ClassDecl || kind == CXCursor_StructDecl || kind == CXCursor_UnionDecl
            || kind == CXCursor_ClassTemplate || kind == CXCursor_ClassTemplatePartialSpecialization;
    }

    cpp_linkage convert_linkage(bool class_var, CXType type, CX_StorageClass storage) STANDARDESE_NOEXCEPT
    {
        switch (storage)
        {
            case CX_SC_None:
                return clang_isConstQualifiedType(type) && !clang_isVolatileQualifiedType(type)
                      ? cpp_internal_linkage : cpp_no_linkage;
            case CX_SC_Extern:
                return cpp_external_linkage;
            case CX_SC_Static:
                return class_var ? cpp_external_linkage : cpp_internal_linkage;
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

    bool is_variable_mutable(cpp_cursor cur, const cpp_name &name)
    {
        return detail::has_prefix_token(cur, "mutable", name.c_str());
    }
}

cpp_ptr<cpp_variable> cpp_variable::parse(cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_VarDecl);

    auto name = detail::parse_name(cur);

    std::string initializer;
    auto type = parse_variable_type(cur, name, initializer);

    auto linkage = convert_linkage(is_variable_static_class(cur), type.get_type(), clang_Cursor_getStorageClass(cur));
    auto is_thread_local = is_variable_thread_local(cur, name);

    return detail::make_ptr<cpp_variable>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                          std::move(type), std::move(initializer), linkage, is_thread_local);
}

cpp_ptr<cpp_member_variable> cpp_member_variable::parse(cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_FieldDecl);

    auto name = detail::parse_name(cur);

    std::string initializer;
    auto type = parse_variable_type(cur, name, initializer);

    auto linkage = convert_linkage(false, type.get_type(), clang_Cursor_getStorageClass(cur));
    auto is_thread_local = is_variable_thread_local(cur, name);
    auto is_mutable = is_variable_mutable(cur, name);

    return detail::make_ptr<cpp_member_variable>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                                 std::move(type), std::move(initializer), linkage,
                                                 is_mutable, is_thread_local);
}
