// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/quote.hpp>

#include <catch.hpp>

#include <standardese/markup/generator.hpp>
#include <standardese/markup/paragraph.hpp>

using namespace standardese::markup;

TEST_CASE("block_quote", "[markup]")
{
    auto html = R"(<blockquote id="standardese-foo">
<p>some text</p>
<p>some more text</p>
</blockquote>
)";

    auto xml = R"(<block-quote id="foo">
<paragraph>some text</paragraph>
<paragraph>some more text</paragraph>
</block-quote>
)";

    block_quote::builder builder(block_id("foo"));
    builder.add_child(paragraph::builder().add_child(text::build("some text")).finish());
    builder.add_child(paragraph::builder().add_child(text::build("some more text")).finish());

    auto ptr = builder.finish()->clone();
    REQUIRE(as_html(*ptr) == html);
    REQUIRE(as_xml(*ptr) == xml);
}
