// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/heading.hpp>

#include <catch.hpp>

using namespace standardese::markup;

TEST_CASE("heading", "[markup]")
{
    auto html = R"(<h4>A <em>heading</em>!</h4>
)";

    heading::builder builder(block_id{});
    builder.add_child(text::build("A "));
    builder.add_child(emphasis::build("heading"));
    builder.add_child(text::build("!"));
    REQUIRE(as_html(*builder.finish()) == html);
}

TEST_CASE("subheading", "[markup]")
{
    auto html = R"(<h5>A <em>subheading</em>!</h5>
)";

    subheading::builder builder(block_id{});
    builder.add_child(text::build("A "));
    builder.add_child(emphasis::build("subheading"));
    builder.add_child(text::build("!"));
    REQUIRE(as_html(*builder.finish()) == html);
}
