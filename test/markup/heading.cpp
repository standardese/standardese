// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/heading.hpp>

#include "../external/catch/single_include/catch2/catch.hpp"

#include <standardese/markup/generator.hpp>

using namespace standardese::markup;

TEST_CASE("heading", "[markup]")
{
    auto html = R"(<h4>A <em>heading</em>!</h4>
)";
    auto xml  = R"(<heading>A <emphasis>heading</emphasis>!</heading>
)";
    auto md   = R"(#### A *heading*\!
)";

    heading::builder builder(block_id{});
    builder.add_child(text::build("A "));
    builder.add_child(emphasis::build("heading"));
    builder.add_child(text::build("!"));

    auto ptr = builder.finish();
    REQUIRE(as_html(*ptr) == html);
    REQUIRE(as_xml(*ptr->clone()) == xml);
    REQUIRE(as_markdown(*ptr) == md);
}

TEST_CASE("subheading", "[markup]")
{
    auto html = R"(<h5>A <em>subheading</em>!</h5>
)";
    auto xml  = R"(<subheading>A <emphasis>subheading</emphasis>!</subheading>
)";
    auto md   = R"(##### A *subheading*\!
)";

    subheading::builder builder(block_id{});
    builder.add_child(text::build("A "));
    builder.add_child(emphasis::build("subheading"));
    builder.add_child(text::build("!"));

    auto ptr = builder.finish();
    REQUIRE(as_html(*ptr) == html);
    REQUIRE(as_xml(*ptr->clone()) == xml);
    REQUIRE(as_markdown(*ptr) == md);
}
