// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/tokenizer.hpp>

#include <standardese/error.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

void detail::skip_whitespace(token_stream& stream)
{
    while (std::isspace(stream.peek().get_value()[0]))
        stream.bump();
}

void detail::skip(token_stream& stream, const cpp_cursor& cur, const char* value)
{
    if (!*value)
        return;

    auto& val = stream.peek();
    if (val.get_value() != value)
        throw parse_error(source_location(cur), std::string("expected \'") + value + "\' got \'"
                                                    + val.get_value().c_str() + "\'");
    stream.bump();
}

void detail::skip(token_stream& stream, const cpp_cursor& cur,
                  std::initializer_list<const char*> values)
{
    for (auto val : values)
    {
        skip(stream, cur, val);
        skip_whitespace(stream);
    }
}

bool detail::skip_if_token(detail::token_stream& stream, const char* token)
{
    if (stream.peek().get_value() != token)
        return false;
    stream.bump();
    detail::skip_whitespace(stream);
    return true;
}

void detail::skip_attribute(detail::token_stream& stream, const cpp_cursor& cur)
{
    if (stream.peek().get_value() == "[" && stream.peek(1).get_value() == "[")
    {
        stream.bump(); // opening
        skip_bracket_count(stream, cur, "[", "]");
        stream.bump(); // closing
    }
    else if (skip_if_token(stream, "__attribute__"))
    {
        skip(stream, cur, "(");
        skip_bracket_count(stream, cur, "(", ")");
        skip(stream, cur, ")");
    }
}

detail::tokenizer::tokenizer(const translation_unit& tu, cpp_cursor cur) : tu_(tu.get_cxunit())
{
    clang_tokenize(tu_, clang_getCursorExtent(cur), &tokens_, &no_tokens_);
}

detail::tokenizer::~tokenizer() STANDARDESE_NOEXCEPT
{
    clang_disposeTokens(tu_, tokens_, no_tokens_);
}
