// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/phrasing.hpp>

#include <catch.hpp>

using namespace standardese::markup;

TEST_CASE("text", "[markup]")
{
    auto a = text::build("Hello World!");
    REQUIRE(as_html(*a) == "Hello World!");

    auto b = text::build("Hello\nWorld!");
    REQUIRE(as_html(*b) == "Hello\nWorld!");

    auto c = text::build("<html>&\"'</html>");
    REQUIRE(as_html(*c) == "&lt;html&gt;&amp;&quot;&#x27;&lt;&#x2F;html&gt;");
}

template <typename T>
void test_phrasing(const std::string& open, const std::string& close)
{
    auto a = T::build("foo");
    REQUIRE(as_html(*a) == open + "foo" + close);

    typename T::builder b;
    b.add_child(text::build("foo"));
    b.add_child(text::build("bar"));
    REQUIRE(as_html(*b.finish()) == open + "foobar" + close);

    typename T::builder c;
    c.add_child(emphasis::build("foo"));
    c.add_child(text::build(">bar"));
    REQUIRE(as_html(*c.finish()) == open + "<em>foo</em>&gt;bar" + close);
}

TEST_CASE("emphasis", "[markup]")
{
    test_phrasing<emphasis>("<em>", "</em>");
}

TEST_CASE("strong_emphasis", "[markup]")
{
    test_phrasing<strong_emphasis>("<strong>", "</strong>");
}

TEST_CASE("code", "[markup]")
{
    test_phrasing<code>("<code>", "</code>");
}
