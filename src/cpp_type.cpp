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
: enum_(new cpp_enum(std::move(scope), detail::parse_name(cur), detail::parse_comment(cur)))
{
    if (is_enum_scoped(cur, enum_->get_name()))
        enum_->is_scoped_ = true;

    string spelling(clang_getTypeSpelling(clang_getEnumDeclIntegerType(cur)));
    enum_->type_calculated_ = spelling.get();

    enum_->type_given_ = detail::cat_tokens_after(cur, ":", "{");
}

cpp_entity_ptr cpp_enum::parser::finish(const standardese::parser &par)
{
    par.register_type(*enum_);
    return std::move(enum_);
}
