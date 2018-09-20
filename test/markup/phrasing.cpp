// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/phrasing.hpp>

#include <catch.hpp>

#include <standardese/markup/generator.hpp>

using namespace standardese::markup;

TEST_CASE("text", "[markup]")
{
    auto a = text::build("Hello World!")->clone();
    REQUIRE(as_html(*a) == "Hello World!");
    REQUIRE(as_xml(*a) == as_html(*a));
    REQUIRE(as_markdown(*a) == "Hello World\\!\n");

    auto b = text::build("Hello\nWorld!");
    REQUIRE(as_html(*b) == "Hello\nWorld!");
    REQUIRE(as_xml(*b) == as_html(*b));
    REQUIRE(as_markdown(*b) == "Hello\nWorld\\!\n");

    auto c = text::build("<html>&\"'</html>");
    REQUIRE(as_html(*c) == "&lt;html&gt;&amp;&quot;&#x27;&lt;&#x2F;html&gt;");
    REQUIRE(as_xml(*c) == "&lt;html&gt;&amp;&quot;&apos;&lt;/html&gt;");
    REQUIRE(as_markdown(*c) == R"(\<html\>&"'\</html\>
)");
}

template <typename T>
void test_phrasing(const std::string& html, const std::string& xml, const std::string& markdown)
{
    auto tag_str = [](const std::string& tag, const char* content) {
        return "<" + tag + ">" + content + "</" + tag + ">";
    };

    auto markdownify_str
        = [](const std::string& tag, const char* content) { return tag + content + tag + "\n"; };

    auto a = T::build("foo")->clone();
    REQUIRE(as_html(*a) == tag_str(html, "foo"));
    REQUIRE(as_xml(*a) == tag_str(xml, "foo"));
    REQUIRE(as_markdown(*a) == markdownify_str(markdown, "foo"));

    typename T::builder b;
    b.add_child(text::build("foo"));
    b.add_child(text::build("bar"));

    auto b_ptr = b.finish()->clone();
    REQUIRE(as_html(*b_ptr) == tag_str(html, "foobar"));
    REQUIRE(as_xml(*b_ptr) == tag_str(xml, "foobar"));
    REQUIRE(as_markdown(*b_ptr) == markdownify_str(markdown, "foobar"));

    typename T::builder c;
    c.add_child(emphasis::build("foo"));
    c.add_child(text::build(">bar"));

    auto c_ptr = c.finish()->clone();
    REQUIRE(as_html(*c_ptr) == tag_str(html, "<em>foo</em>&gt;bar"));
    REQUIRE(as_xml(*c_ptr) == tag_str(xml, "<emphasis>foo</emphasis>&gt;bar"));
    if (!std::is_same<T, code>::value)
        REQUIRE(as_markdown(*c_ptr) == markdownify_str(markdown, "*foo*\\>bar"));
}

TEST_CASE("emphasis", "[markup]")
{
    test_phrasing<emphasis>("em", "emphasis", "*");
}

TEST_CASE("strong_emphasis", "[markup]")
{
    test_phrasing<strong_emphasis>("strong", "strong-emphasis", "**");
}

TEST_CASE("code", "[markup]")
{
    test_phrasing<code>("code", "code", "`");
}

TEST_CASE("verbatim", "[markup]")
{
    auto v = verbatim::build("*Hello* <i>World</i>!");
    REQUIRE(as_html(*v) == "*Hello* <i>World</i>!");
    REQUIRE(as_xml(*v) == "<verbatim>*Hello* &lt;i&gt;World&lt;/i&gt;!</verbatim>");
    REQUIRE(as_markdown(*v) == "*Hello* <i>World</i>!\n");
}

TEST_CASE("soft_break", "[markup]")
{
    REQUIRE(as_html(*soft_break::build()) == "\n");
    REQUIRE(as_xml(*soft_break::build()) == "<soft-break></soft-break>\n");
    REQUIRE(as_markdown(*soft_break::build()) == " \n");
}

TEST_CASE("hard_break", "[markup]")
{
    REQUIRE(as_html(*hard_break::build()) == "<br/>\n");
    REQUIRE(as_xml(*hard_break::build()) == "<hard-break></hard-break>\n");
    REQUIRE(as_markdown(*hard_break::build()) == "  \n");
}
