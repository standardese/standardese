// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_variable.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/error.hpp>

using namespace standardese;

namespace
{
    cpp_type_ref parse_variable_type(translation_unit &tu, cpp_cursor cur,
                                     const cpp_name &name, std::string &initializer,
                                     bool &is_thread_local, bool &is_mutable)
    {
        assert(clang_getCursorKind(cur) == CXCursor_VarDecl
             || clang_getCursorKind(cur) == CXCursor_FieldDecl);

        is_thread_local = is_mutable = false;

        auto type = clang_getCursorType(cur);
        cpp_name type_name;

        detail::tokenizer tokenizer(tu, cur);
        auto stream = detail::make_stream(tokenizer);
        auto location = source_location(clang_getCursorLocation(cur), name);

        for (auto in_type = true, was_bitfield = false; stream.peek().get_value() != ";";)
        {
            detail::skip_attribute(stream, location);

            if (detail::skip_if_token(stream, name.c_str())
                || detail::skip_if_token(stream, "extern")
                || detail::skip_if_token(stream, "static"))
                // ignore
                continue;
            else if (detail::skip_if_token(stream, "thread_local"))
                is_thread_local = true;
            else if (detail::skip_if_token(stream, "mutable"))
                is_mutable = true;
            else if (detail::skip_if_token(stream, ":"))
                was_bitfield = true;
            else if (was_bitfield)
            {
                stream.bump();
                was_bitfield = false;
                detail::skip_whitespace(stream);
            }
            else if (detail::skip_if_token(stream, "="))
                in_type = false;
            else
                (in_type ? type_name : initializer) += stream.get().get_value().c_str();
        }

        while (std::isspace(type_name.back()))
            type_name.pop_back();

        while (std::isspace(initializer.back()))
            initializer.pop_back();

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
}

cpp_ptr<cpp_variable> cpp_variable::parse(translation_unit &tu, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_VarDecl);

    auto name = detail::parse_name(cur);
    source_location location(clang_getCursorLocation(cur), name);

    std::string initializer;
    bool is_thread_local, is_mutable;
    auto type = parse_variable_type(tu, cur, name, initializer, is_thread_local, is_mutable);
    if (is_mutable)
        throw parse_error(location, "non-member variable is mutable");

    auto linkage = convert_linkage(is_variable_static_class(cur), type.get_type(), clang_Cursor_getStorageClass(cur));

    return detail::make_ptr<cpp_variable>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                          std::move(type), std::move(initializer), linkage, is_thread_local);
}

cpp_ptr<cpp_member_variable> cpp_member_variable::parse(translation_unit &tu, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_FieldDecl);

    auto name = detail::parse_name(cur);

    std::string initializer;
    bool is_thread_local, is_mutable;
    auto type = parse_variable_type(tu, cur, name, initializer, is_thread_local, is_mutable);

    auto linkage = convert_linkage(false, type.get_type(), clang_Cursor_getStorageClass(cur));

    if (clang_Cursor_isBitField(cur))
    {
        auto no_bits = clang_getFieldDeclBitWidth(cur);
        return detail::make_ptr<cpp_bitfield>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                             std::move(type), std::move(initializer), no_bits, linkage,
                                             is_mutable, is_thread_local);
    }
    return detail::make_ptr<cpp_member_variable>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                                 std::move(type), std::move(initializer), linkage,
                                                 is_mutable, is_thread_local);
}
