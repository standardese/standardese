// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/output.hpp>

#include <cassert>

using namespace standardese;

void markdown_output::write_header_begin(unsigned level)
{
    assert(level <= 6);

    static const char str[] = "######";
    get_output().write_str(str, level);
}

void markdown_output::write_header_end(unsigned level)
{
    assert(level <= 6);
    get_output().write_blank_line();
}

void markdown_output::write_begin(style s)
{
    switch (s)
    {
        case style::normal:
            break;
        case style::italics:
            get_output().write_char('*');
            break;
        case style::bold:
            get_output().write_str("**", 2);
            break;
        case style::code_span:
            // use two to allow backticks inside
            get_output().write_str("``", 2);
            break;
        case style::code_block:
            get_output().write_blank_line();
            get_output().write_str("```cpp\n", 7);
            break;
    }
}

void markdown_output::write_end(style s)
{
    if (s == style::code_block)
    {
        get_output().write_new_line();
        get_output().write_str("```\n\n", 5);
    }
    else
        write_begin(s);
}

void markdown_output::write_paragraph_begin() {}

void markdown_output::write_paragraph_end()
{
    get_output().write_blank_line();
}
