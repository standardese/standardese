// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/output.hpp>

#include <algorithm>
#include <cassert>

using namespace standardese;

output_base::~output_base() STANDARDESE_NOEXCEPT
{
}

void markdown_output::write_header_begin(unsigned level)
{
    static const char str[] = "######";
    get_output().write_str(str, std::min(level, 6u));
    get_output().write_char(' ');
}

void markdown_output::write_header_end(unsigned)
{
    get_output().write_blank_line();
}

void markdown_output::do_write_seperator()
{
    get_output().write_blank_line();
    get_output().write_str("---", 3);
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
        get_output().write_char('`');
        break;
    default:
        break;
    }
}

void markdown_output::write_paragraph_begin()
{
}

void markdown_output::write_paragraph_end()
{
    get_output().write_blank_line();
}

void markdown_output::write_code_block_begin()
{
    get_output().write_blank_line();
    get_output().write_str("```cpp\n", 7);
}

void markdown_output::write_code_block_end()
{
    get_output().write_new_line();
    get_output().write_str("```", 3);
    get_output().write_blank_line();
}

void markdown_output::do_write_section_heading(const std::string& section_name)
{
    get_output().write_char('*');
    get_output().write_str(section_name.c_str(), section_name.size());
    get_output().write_str(":* ", 3);
}
