// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/tokenizer.hpp>

#include <cassert>
#include <fstream>
#include <string>

#include <standardese/cpp_cursor.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

void detail::skip_whitespace(token_stream &stream)
{
    while (std::isspace(stream.peek().get_value()[0]))
        stream.bump();
}

void detail::skip(token_stream &stream, const source_location &location, const char *value)
{
    auto& val = stream.peek();
    if (val.get_value() != value)
        throw parse_error(location, std::string("expected \'") + value + "\' got \'" + val.get_value().c_str() + "\'");
    stream.bump();
}

void detail::skip(token_stream &stream, const source_location &location, std::initializer_list<const char *> values)
{
    for (auto val : values)
    {
        skip(stream, location, val);
        skip_whitespace(stream);
    }
}

bool detail::skip_if_token(detail::token_stream &stream, const char *token)
{
    if (stream.peek().get_value() != token)
        return false;
    stream.bump();
    detail::skip_whitespace(stream);
    return true;
}

std::string detail::tokenizer::read_source(cpp_cursor cur)
{
    // query location
    auto source = clang_getCursorExtent(cur);
    auto begin = clang_getRangeStart(source);
    auto end = clang_getRangeEnd(source);

    // translate location into offset and file
    CXFile file;
    unsigned begin_offset = 0u, end_offset = 0u;
    clang_getSpellingLocation(begin, &file, nullptr, nullptr, &begin_offset);
    clang_getSpellingLocation(end, nullptr, nullptr, nullptr, &end_offset);
    assert(end_offset > begin_offset);

    // open file buffer
    std::filebuf buf;

    string filename(clang_getFileName(file));
    buf.open(filename, std::ios_base::in | std::ios_base::binary);
    assert(buf.is_open());

    // seek to beginning
    buf.pubseekpos(begin_offset);

    // read bytes
    std::string result(end_offset - begin_offset, '\0');
    buf.sgetn(&result[0], result.size());

    // awesome libclang bug:
    // if there is a macro expansion at the end, the closing bracket is missing
    // ie.: using foo = IMPL_DEFINED(bar
    // go backwards, if a '(' is found before a ')', append a ')'
    for (auto iter = result.rbegin(); iter != result.rend(); ++iter)
    {
        if (*iter == ')')
            break;
        else if (*iter == '(')
        {
            result += ')';
            break;
        }
    }

    return result;
}

namespace standardese { namespace detail
{
    struct context_impl;

    context_impl& get_context_impl(translation_unit &tu);

    context& get_preprocessing_context(context_impl &impl);
}} // namespace standardese::detail

detail::tokenizer::tokenizer(translation_unit &tu, cpp_cursor cur)
: source_(read_source(cur)), impl_(&get_preprocessing_context(get_context_impl(tu)))
{
    // append trailing newline
    // required for parsing code
    source_ += '\n';
}
