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

void detail::skip(token_stream &stream, const char *value)
{
    auto& val = stream.peek();
    assert(val.get_value() == value);
    stream.bump();
}

void detail::skip(token_stream &stream, std::initializer_list<const char *> values)
{
    for (auto val : values)
    {
        skip(stream, val);
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

namespace
{
    std::string read_source(cpp_cursor cur, const char *filename)
    {
        // query location
        auto source = clang_getCursorExtent(cur);
        auto begin = clang_getRangeStart(source);
        auto end = clang_getRangeEnd(source);

        // translate location into offset
        unsigned begin_offset = 0u, end_offset = 0u;
        clang_getFileLocation(begin, nullptr, nullptr, nullptr, &begin_offset);
        clang_getFileLocation(end, nullptr, nullptr, nullptr, &end_offset);
        assert(end_offset > begin_offset);

        // open file buffer
        std::filebuf buf;
        buf.open(filename, std::ios_base::in | std::ios_base::binary);
        assert(buf.is_open());

        // seek to beginning
        buf.pubseekpos(begin_offset);

        // read bytes
        std::string result(end_offset - begin_offset, '\0');
        buf.sgetn(&result[0], result.size());

        return result;
    }
}

detail::tokenizer::tokenizer(translation_unit &tu, cpp_cursor cur)
: source_(read_source(cur, tu.get_path())), impl_(&tu.get_preprocessing_context())
{
    // append trailing newline
    // required for Boost.Wave
    source_ += '\n';
}
