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
    cpp_type_ref parse_enum_underlying(translation_unit &tu, cpp_cursor cur,
                                       const cpp_name &name,
                                       bool &is_definition, bool &is_scoped)
    {
        assert(clang_getCursorKind(cur) == CXCursor_EnumDecl);

        detail::tokenizer tokenizer(tu, cur);
        auto stream = detail::make_stream(tokenizer);
        source_location location(clang_getCursorLocation(cur), name);

        cpp_name underlying;

        detail::skip(stream, location, {"enum"});

        if (stream.peek().get_value() == "class")
        {
            stream.bump();
            detail::skip_whitespace(stream);
            is_scoped = true;
        }

        detail::skip_attribute(stream, location);
        detail::skip_whitespace(stream);
        detail::skip(stream, location, {name.c_str()});

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
                    underlying += spelling.c_str();
            }

            while (std::isspace(underlying.back()))
                underlying.pop_back();
        }
        else if (stream.peek().get_value() == "{")
        {
            is_definition = true;
        }

        return {clang_getEnumDeclIntegerType(cur), underlying};
    }

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

cpp_ptr<cpp_enum_value> cpp_enum_value::parse(translation_unit &tu, cpp_name scope, cpp_cursor cur)
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

    result->explicit_ = is_explicit_value(tu, cur);

    return result;
}

cpp_enum::parser::parser(translation_unit &tu, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_EnumDecl);

    auto name = detail::parse_name(cur);
    auto type = clang_getCursorType(cur);

    bool is_definition = false, is_scoped = false;
    auto underlying = parse_enum_underlying(tu, cur, name, is_definition, is_scoped);

    if (is_definition)
    {
        enum_ = cpp_ptr<cpp_enum>(new cpp_enum(std::move(scope), std::move(name), detail::parse_comment(cur),
                                               type, std::move(underlying)));

        if (is_scoped)
            enum_->is_scoped_ = true;
    }
}

cpp_entity_ptr cpp_enum::parser::finish(const standardese::parser &par)
{
    if (enum_)
        par.register_type(*enum_);
    return std::move(enum_);
}

void cpp_enum::parser::add_entity(cpp_entity_ptr ptr)
{
    assert(enum_);
    auto val = static_cast<cpp_enum_value*>(ptr.release());
    enum_->add_enum_value(cpp_ptr<cpp_enum_value>(val));
}

cpp_name cpp_enum::parser::scope_name()
{
    return enum_ && enum_->is_scoped_ ? enum_->get_name() : "";
}