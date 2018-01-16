// Copyright (C) 2016-2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/block.hpp>

using namespace standardese::markup;

namespace
{
    constexpr bool is_alpha(char c)
    {
        // assuming ASCII
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    constexpr bool is_digit(char c)
    {
        return c >= '0' && c <= '9';
    }

    void escape_char(std::string& str, char c)
    {
        if (is_alpha(c) || is_digit(c) || c == '_' || c == '-')
            str += c;
        // distinction is somewhat arbitrary
        else if (c == ':')
            str += '_';
        else
            str += '-';
    }
}

std::string block_id::as_output_str() const
{
    std::string id;
    id.reserve(id_.size());
    for (auto c : id_)
        escape_char(id, c);
    return id;
}
