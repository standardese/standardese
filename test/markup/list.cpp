// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/list.hpp>

#include <catch.hpp>

#include <standardese/markup/paragraph.hpp>

using namespace standardese::markup;

TEST_CASE("unordered_list", "[markup]")
{
    auto html = R"(<ul id="standardese-list">
<li>
<p></p>
<p></p>
</li>
<li>
<p>text</p>
</li>
</ul>
)";

    unordered_list::builder builder(block_id("list"));
    builder.add_item(list_item::builder()
                         .add_child(paragraph::builder().finish())
                         .add_child(paragraph::builder().finish())
                         .finish());
    builder.add_item(paragraph::builder().add_child(text::build("text")).finish());
    REQUIRE(as_html(*builder.finish()) == html);
}

TEST_CASE("ordered_list", "[markup]")
{
    auto html = R"(<ol id="standardese-list">
<li>
<p></p>
<p></p>
</li>
<li>
<p>text</p>
</li>
</ol>
)";

    ordered_list::builder builder(block_id("list"));
    builder.add_item(list_item::builder()
                         .add_child(paragraph::builder().finish())
                         .add_child(paragraph::builder().finish())
                         .finish());
    builder.add_item(paragraph::builder().add_child(text::build("text")).finish());
    REQUIRE(as_html(*builder.finish()) == html);
}
