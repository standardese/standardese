// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/link.hpp>

#include <catch.hpp>

#include <standardese/markup/document.hpp>
#include <standardese/markup/generator.hpp>
#include <standardese/markup/paragraph.hpp>

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

TEST_CASE("internal_link", "[markup]")
{
    auto doc1 = [] {
        template_document::builder builder("foo", "doc1.templ");
        builder.add_child(paragraph::builder(block_id("p1")).finish());

        paragraph::builder p2(block_id("p2"));
        p2.add_child(internal_link::builder("title", block_reference(block_id("p1")))
                         .add_child(text::build("link 1"))
                         .finish());
        builder.add_child(p2.finish());

        return builder.finish();
    }();

    auto doc1_html = R"(<section class="standardese-template-document">
<p id="standardese-p1"></p>
<p id="standardese-p2"><a href="#standardese-p1" title="title">link 1</a></p>
</section>
)";
    REQUIRE(as_html(*doc1) == doc1_html);

    internal_link::builder builder(
        block_reference(output_name::from_file_name("doc1.templ"), block_id("p1")));
    builder.add_child(text::build("link 2"));
    REQUIRE(as_html(*builder.finish()) == R"(<a href="doc1.templ#standardese-p1">link 2</a>)");

    // non existing link, but doesn't matter
    internal_link::builder builder2(
        block_reference(output_name::from_name("doc2"), block_id("p3")));
    builder2.add_child(text::build("link 3"));
    REQUIRE(as_html(*builder2.finish()) == R"(<a href="doc2.html#standardese-p3">link 3</a>)");
}
