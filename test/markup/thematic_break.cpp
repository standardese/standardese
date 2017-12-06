// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/thematic_break.hpp>

#include <catch.hpp>

#include <standardese/markup/generator.hpp>

using namespace standardese::markup;

TEST_CASE("thematic_break", "[markup]")
{
    REQUIRE(as_html(*thematic_break::build()) == "<hr />\n");
    REQUIRE(as_xml(*thematic_break::build()->clone()) == "<thematic-break></thematic-break>\n");
    REQUIRE(as_markdown(*thematic_break::build()) == "-----\n");
}
