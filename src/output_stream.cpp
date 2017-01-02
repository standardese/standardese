// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/output_stream.hpp>

using namespace standardese;

output_stream_base::~output_stream_base() STANDARDESE_NOEXCEPT
{
}

void output_stream_base::write_str(const char* str, std::size_t n)
{
    for (std::size_t i = 0u; i != n; ++i)
        write_char(str[i]);
}

void output_stream_base::write_char(char c)
{
    do_indent();

    do_write_char(c);
    last_ = c;
}

void output_stream_base::indent(unsigned width)
{
    level_ += width;
}

void output_stream_base::unindent(unsigned width)
{
    assert(level_ >= width);
    level_ -= width;
}

void output_stream_base::remove_trailing_line()
{
    if (last_ == '\n')
    {
        last_ = undo_write();
        if (last_ == ' ') // only if there is intendation to remove
            for (auto i = 0u; i != level_; ++i)
                last_ = undo_write();
        else
            // there wasn't an empty line
            write_char('\n');
    }
}

void output_stream_base::do_indent()
{
    if (last_ == '\n')
    {
        for (auto i = 0u; i != level_; ++i)
            do_write_char(' ');
        last_ = ' ';
    }
}