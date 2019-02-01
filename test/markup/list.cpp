// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/list.hpp>

#include <catch.hpp>

#include <algorithm>

#include <standardese/markup/generator.hpp>
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
<li>
<dl class="standardese-term-description-item">
<dt>A term</dt>
<dd>&mdash; A description</dd>
</dl>
</li>
</ul>
)";
    auto xml  = R"(<unordered-list id="list">
<list-item>
<paragraph></paragraph>
<paragraph></paragraph>
</list-item>
<list-item>
<paragraph>text</paragraph>
</list-item>
<term-description-item>
<term>A term</term>
<description>A description</description>
</term-description-item>
</unordered-list>
)";
    auto md   = std::string(R"(  -$

  - text

  - A term &mdash; A description
)");
    std::replace(md.begin(), md.end(), '$', ' ');

    unordered_list::builder builder(block_id("list"));
    builder.add_item(list_item::builder()
                         .add_child(paragraph::builder().finish())
                         .add_child(paragraph::builder().finish())
                         .finish());
    builder.add_item(
        list_item::build(paragraph::builder().add_child(text::build("text")).finish()));
    builder.add_item(
        term_description_item::build(block_id(), term::build(text::build("A term")),
                                     description::build(text::build("A description"))));

    auto ptr = builder.finish()->clone();
    REQUIRE(as_html(*ptr) == html);
    REQUIRE(as_xml(*ptr) == xml);
    REQUIRE(as_markdown(*ptr) == md);
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
    auto xml  = R"(<ordered-list id="list">
<list-item>
<paragraph></paragraph>
<paragraph></paragraph>
</list-item>
<list-item>
<paragraph>text</paragraph>
</list-item>
</ordered-list>
)";
    auto md   = std::string(R"(1.$$

2.  text
)");
    std::replace(md.begin(), md.end(), '$', ' ');

    ordered_list::builder builder(block_id("list"));
    builder.add_item(list_item::builder()
                         .add_child(paragraph::builder().finish())
                         .add_child(paragraph::builder().finish())
                         .finish());
    builder.add_item(
        list_item::build(paragraph::builder().add_child(text::build("text")).finish()));
    auto ptr = builder.finish()->clone();
    REQUIRE(as_html(*ptr) == html);
    REQUIRE(as_xml(*ptr) == xml);
    REQUIRE(as_markdown(*ptr) == md);
}
