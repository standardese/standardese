// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/documentation.hpp>

#include <catch.hpp>

using namespace standardese::markup;

TEST_CASE("file_documentation", "[markup]")
{
    auto html = R"(<article id="standardese-a" class="standardese-file-documentation">
<section id="standardese-foo" class="standardese-entity-documentation"></section>
</article>
)";

    file_documentation::builder builder(block_id("a"), "a");
    builder.add_child(entity_documentation::builder(block_id("foo")).finish());
    REQUIRE(as_html(*builder.finish()) == html);
}
