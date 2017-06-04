// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/phrasing.hpp>

#include <catch.hpp>

#include <standardese/markup/generator.hpp>

using namespace standardese::markup;

TEST_CASE("text", "[markup]")
{
    auto a = text::build("Hello World!");
    REQUIRE(as_html(*a) == "Hello World!");
    REQUIRE(as_xml(*a) == as_html(*a));

    auto b = text::build("Hello\nWorld!");
    REQUIRE(as_html(*b) == "Hello\nWorld!");
    REQUIRE(as_xml(*b) == as_html(*b));

    auto c = text::build("<html>&\"'</html>");
    REQUIRE(as_html(*c) == "&lt;html&gt;&amp;&quot;&#x27;&lt;&#x2F;html&gt;");
    REQUIRE(as_xml(*c) == "&lt;html&gt;&amp;&quot;&apos;&lt;/html&gt;");
}

template <typename T>
void test_phrasing(const std::string& html, const std::string& xml)
{
    auto tag_str = [](const std::string& tag, const char* content) {
        return "<" + tag + ">" + content + "</" + tag + ">";
    };

    auto a = T::build("foo");
    REQUIRE(as_html(*a) == tag_str(html, "foo"));
    REQUIRE(as_xml(*a) == tag_str(xml, "foo"));

    typename T::builder b;
    b.add_child(text::build("foo"));
    b.add_child(text::build("bar"));

    auto b_ptr = b.finish();
    REQUIRE(as_html(*b_ptr) == tag_str(html, "foobar"));
    REQUIRE(as_xml(*b_ptr) == tag_str(xml, "foobar"));

    typename T::builder c;
    c.add_child(emphasis::build("foo"));
    c.add_child(text::build(">bar"));

    auto c_ptr = c.finish();
    REQUIRE(as_html(*c_ptr) == tag_str(html, "<em>foo</em>&gt;bar"));
    REQUIRE(as_xml(*c_ptr) == tag_str(xml, "<emphasis>foo</emphasis>&gt;bar"));
}

TEST_CASE("emphasis", "[markup]")
{
    test_phrasing<emphasis>("em", "emphasis");
}

TEST_CASE("strong_emphasis", "[markup]")
{
    test_phrasing<strong_emphasis>("strong", "strong-emphasis");
}

TEST_CASE("code", "[markup]")
{
    test_phrasing<code>("code", "code");
}

TEST_CASE("soft_break", "[markup]")
{
    REQUIRE(as_html(*soft_break::build()) == "\n");
    REQUIRE(as_xml(*soft_break::build()) == "<soft-break></soft-break>\n");
}

TEST_CASE("hard_break", "[markup]")
{
    REQUIRE(as_html(*hard_break::build()) == "<br/>\n");
    REQUIRE(as_xml(*hard_break::build()) == "<hard-break></hard-break>\n");
}
