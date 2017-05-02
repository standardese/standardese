// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/documentation.hpp>

#include <catch.hpp>

#include <standardese/markup/document.hpp>
#include <standardese/markup/generator.hpp>
#include <standardese/markup/paragraph.hpp>

using namespace standardese::markup;

TEST_CASE("file_documentation", "[markup]")
{
    SECTION("no heading")
    {
        auto html = R"(<article id="standardese-a" class="standardese-file-documentation">
<section id="standardese-foo" class="standardese-entity-documentation">
</section>
<p>foo</p>
</article>
)";

        file_documentation::builder builder(block_id("a"));
        builder.add_child(entity_documentation::builder(block_id("foo")).finish());
        builder.add_child(paragraph::builder(block_id("")).add_child(text::build("foo")).finish());
        REQUIRE(as_html(*builder.finish()) == html);
    }
    SECTION("heading")
    {
        auto html = R"(<article class="standardese-file-documentation">
<h1 class="standardese-file-documentation-heading">A heading!</h1>
<p>foo</p>
</article>
)";

        file_documentation::builder builder(block_id(), heading::build(block_id(), "A heading!"));
        builder.add_child(paragraph::builder().add_child(text::build("foo")).finish());
        REQUIRE(as_html(*builder.finish()) == html);
    }
}

TEST_CASE("entity_documentation", "[markup]")
{
    SECTION("no heading")
    {
        auto html = R"(<section id="standardese-a" class="standardese-entity-documentation">
<p>foo</p>
</section>
)";

        entity_documentation::builder builder(block_id("a"));
        builder.add_child(paragraph::builder(block_id("")).add_child(text::build("foo")).finish());
        REQUIRE(as_html(*builder.finish()) == html);
    }
    SECTION("heading")
    {
        auto html = R"(<section class="standardese-template-document">
<article class="standardese-file-documentation">
<section class="standardese-entity-documentation">
<h2 class="standardese-entity-documentation-heading">1</h2>
<section class="standardese-entity-documentation">
<h3 class="standardese-entity-documentation-heading">2</h3>
</section>
<hr class="standardese-entity-documentation-break" />
</section>
<hr class="standardese-entity-documentation-break" />
</article>
</section>
)";

        template_document::builder file("", "templ");

        entity_documentation::builder builder(block_id(), heading::build(block_id(), "1"));
        builder.add_child(
            entity_documentation::builder(block_id(), heading::build(block_id(), "2")).finish());
        file.add_child(
            file_documentation::builder(block_id()).add_child(builder.finish()).finish());

        REQUIRE(as_html(*file.finish()) == html);
    }
}
