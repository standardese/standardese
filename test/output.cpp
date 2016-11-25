// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/output.hpp>

#include <catch.hpp>

#include <standardese/index.hpp>

#include "test_parser.hpp"

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

std::string get_text(const std::string& path)
{
    std::ifstream file(path);

    std::string result(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{});
    return result;
}

TEST_CASE("output")
{
    using standardese::index;

    auto code = R"(
        /// A function.
        void foo();

        /// A class.
        struct bar {};
)";

    parser p(test_logger);
    auto   tu = parse(p, "output", code);

    index idx;
    auto  doc_entity = doc_file::parse(p, idx, "my_file", tu.get_file());

    output_format_html format;
    output             out(p, idx, "", format);

    SECTION("raw without URLS")
    {
        auto text = R"(This is just text.
        I'm using a [regular URL here](https://foo.bar/baz). I expect no change in output.)";

        raw_document doc("other_file", text);
        REQUIRE(doc.file_name == "other_file");
        REQUIRE(doc.file_extension == "");

        out.render_raw(p.get_logger(), doc);
        REQUIRE(get_text("other_file.html") == text);
    }
    SECTION("raw with URLS")
    {
        auto text = R"(This is text with a [regular URL](https://foo.bar/baz)
and a special one [here](standardese://foo()/) as well as one [here](standardese://bar/))";

        auto text_written = R"(This is text with a [regular URL](https://foo.bar/baz)
and a special one [here](my_file.html#foo%28%29) as well as one [here](my_file.html#bar))";

        raw_document doc("other_file.md", text);
        REQUIRE(doc.file_name == "other_file");
        REQUIRE(doc.file_extension == "md");

        out.render_raw(p.get_logger(), doc);
        REQUIRE(get_text("other_file.md") == text_written);
    }
}
