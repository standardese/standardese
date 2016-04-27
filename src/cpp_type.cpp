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
    cpp_type_ref parse_alias_target(cpp_cursor cur, const cpp_name &name)
    {
        auto type = clang_getTypedefDeclUnderlyingType(cur);

        if (clang_getCursorKind(cur) == CXCursor_TypeAliasDecl)
            return {type, detail::parse_alias_type_name(cur)};

        assert(clang_getCursorKind(cur) == CXCursor_TypedefDecl);

        auto str = detail::parse_typedef_type_name(cur, name);
        return {type, str};
    }

    cpp_type_ref parse_enum_underlying(cpp_cursor cur, const cpp_name &name)
    {
        assert(clang_getCursorKind(cur) == CXCursor_EnumDecl);

        auto type = clang_getEnumDeclIntegerType(cur);
        auto str = detail::parse_enum_type_name(cur);

        return {type, str};
    }
}

cpp_name cpp_type_ref::get_full_name() const
{
    string spelling(clang_getTypeSpelling(type_));
    return spelling.get();
}

cpp_ptr<cpp_type_alias> cpp_type_alias::parse(const parser &p, const cpp_name &scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_TypedefDecl
           || clang_getCursorKind(cur) == CXCursor_TypeAliasDecl);

    auto name = detail::parse_name(cur);
    auto target = parse_alias_target(cur, name);
    auto result = detail::make_ptr<cpp_type_alias>(scope, std::move(name), detail::parse_comment(cur),
                                                   clang_getCursorType(cur), target);

    p.register_type(*result);

    return result;
}

namespace
{
    bool is_unsigned_integer(CXType t)
    {
        auto kind = t.kind;
        return kind == CXType_Char_U
               || kind == CXType_UChar
               || kind == CXType_Char16
               || kind == CXType_Char32
               || kind == CXType_UShort
               || kind == CXType_UInt
               || kind == CXType_ULong
               || kind == CXType_ULongLong
               || kind == CXType_UInt128;
    }

    bool is_signed_integer(CXType t)
    {
        auto kind = t.kind;
        return kind == CXType_Char_S
               || kind == CXType_SChar
               || kind == CXType_Short
               || kind == CXType_Int
               || kind == CXType_Long
               || kind == CXType_LongLong
               || kind == CXType_Int128;
    }

    bool is_explicit_value(cpp_cursor cur)
    {
        return detail::has_token(cur, "=");
    }
}

cpp_ptr<cpp_enum_value> cpp_enum_value::parse(cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_EnumConstantDecl);

    cpp_ptr<cpp_enum_value> result;
    auto name = detail::parse_name(cur);
    auto comment = detail::parse_comment(cur);

    auto type = clang_getEnumDeclIntegerType(clang_getCursorSemanticParent(cur));
    if (is_signed_integer(type))
    {
        auto val = clang_getEnumConstantDeclValue(cur);
        result = detail::make_ptr<cpp_signed_enum_value>(std::move(scope), std::move(name),
                                                         std::move(comment), val);
    }
    else if (is_unsigned_integer(type))
    {
        auto val = clang_getEnumConstantDeclUnsignedValue(cur);
        result = detail::make_ptr<cpp_unsigned_enum_value>(std::move(scope), std::move(name),
                                                           std::move(comment), val);
    }
    else
        assert(false);

    result->explicit_ = is_explicit_value(cur);

    return result;
}

namespace
{
    bool is_enum_scoped(cpp_cursor cur, const cpp_name &n)
    {
        return detail::has_prefix_token(cur, "class", n.c_str());
    }
}

cpp_enum::parser::parser(cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_EnumDecl);

    auto name = detail::parse_name(cur);
    auto type = clang_getCursorType(cur);
    auto underlying = parse_enum_underlying(cur, name);

    enum_ = cpp_ptr<cpp_enum>(new cpp_enum(std::move(scope), std::move(name), detail::parse_comment(cur),
                                        type, std::move(underlying)));

    if (is_enum_scoped(cur, enum_->get_name()))
        enum_->is_scoped_ = true;
}

cpp_entity_ptr cpp_enum::parser::finish(const standardese::parser &par)
{
    par.register_type(*enum_);
    return std::move(enum_);
}
