// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_enum.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

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

    bool is_explicit_value(translation_unit &tu, cpp_cursor cur)
    {
        detail::tokenizer tokenizer(tu, cur);
        for (auto val : tokenizer)
            if (val.get_value() == "=")
                return true;
        return false;
    }
}

cpp_ptr<cpp_enum_value> cpp_enum_value::parse(translation_unit &tu,
                                              cpp_cursor cur, const cpp_entity &parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_EnumConstantDecl);

    auto is_explicit = is_explicit_value(tu, cur);

    auto type = clang_getEnumDeclIntegerType(clang_getCursorSemanticParent(cur));
    if (is_signed_integer(type))
    {
        auto value = clang_getEnumConstantDeclValue(cur);
        return detail::make_ptr<cpp_signed_enum_value>(cur, parent, value, is_explicit);
    }
    else if (is_unsigned_integer(type))
    {
        auto value = clang_getEnumConstantDeclUnsignedValue(cur);
        return detail::make_ptr<cpp_unsigned_enum_value>(cur, parent, value, is_explicit);
    }

    assert(!"enum type is neither signed nor unsigned");
    return nullptr;
}

cpp_name cpp_enum_value::get_scope() const
{
    if (static_cast<const cpp_enum&>(get_parent()).is_scoped())
        return cpp_entity::get_scope();
    // don't append parent name if enum isn't scoped
    return get_parent().get_scope();
}

namespace
{
    cpp_name parse_underlying_type(detail::token_stream &stream, bool &is_definition)
    {
        std::string underlying_type;
        if (stream.peek().get_value() == ":")
        {
            stream.bump();
            detail::skip_whitespace(stream);

            while (stream.peek().get_value() != ";")
            {
                auto spelling = stream.get().get_value();

                if (spelling == "{")
                {
                    is_definition = true;
                    break;
                }
                else
                    underlying_type += spelling.c_str();
            }


        }
        else if (stream.peek().get_value() == "{")
        {
            is_definition = true;
        }

        detail::erase_trailing_ws(underlying_type);
        return underlying_type;
    }
}

cpp_ptr<cpp_enum> cpp_enum::parse(translation_unit &tu,
                        cpp_cursor cur, const cpp_entity &parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_EnumDecl);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);
    detail::skip(stream, cur, {"enum"});

    auto is_scoped = false;
    if (stream.peek().get_value() == "class")
    {
        stream.bump();
        detail::skip_whitespace(stream);
        is_scoped = true;
    }

    detail::skip_attribute(stream, cur);
    detail::skip_whitespace(stream);
    detail::skip(stream, cur, {detail::parse_name(cur).c_str()});

    auto is_definition = false;
    auto underlying_name = parse_underlying_type(stream, is_definition);
    if (!is_definition)
        return  nullptr;

    auto underlying_type = clang_getEnumDeclIntegerType(cur);

    return detail::make_ptr<cpp_enum>(cur, parent, cpp_type_ref(underlying_name, underlying_type), is_scoped);
}
