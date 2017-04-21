// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/link.hpp>

#include <catch.hpp>

using namespace standardese::markup;

TEST_CASE("external_link", "[markup]")
{
    external_link::builder a("http://foonathan.net/");
    a.add_child(emphasis::build("awesome"));
    a.add_child(text::build(" website"));
    REQUIRE(as_html(*a.finish())
            == "<a href=\"http://foonathan.net/\"><em>awesome</em> website</a>");

    external_link::builder b("title\"", "foo/bar/< &>\n");
    b.add_child(text::build("with title"));
    REQUIRE(as_html(*b.finish())
            == "<a href=\"foo/bar/%3C%20&amp;%3E%0A\" title=\"title&quot;\">with title</a>");
}
