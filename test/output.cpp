// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/output.hpp>

#include <catch.hpp>

using namespace standardese;

TEST_CASE("output_stream_base")
{
    std::ostringstream str;
    streambuf_output   out(*str.rdbuf());

    SECTION("newline test")
    {
        out.write_char('a');
        out.write_new_line();

        out.write_str("b\n", 2);
        out.write_new_line();

        out.write_str("c ignore me", 1);
        out.write_blank_line();

        out.write_str("d\n", 2);
        out.write_blank_line();

        REQUIRE(str.str() == "a\nb\nc\n\nd\n\n");
    }
    SECTION("indentation test")
    {
        out.write_str("a\n", 2);
        out.indent(4);
        out.write_str("b\n", 2);
        out.unindent(4);
        out.write_str("c\n", 2);

        REQUIRE(str.str() == "a\n    b\nc\n");
    }
}
