// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment/parser.hpp>

#include <catch.hpp>

#include <standardese/markup/generator.hpp>

using namespace standardese;
using namespace standardese::comment;

TEST_CASE("parser", "[comment]")
{
    auto comment = R"(A.
A.

B.

C.
C.)";

    auto translated = translate_ast(read_ast(parser(), comment));
    REQUIRE(translated.sections.size() == 1u);

    auto& section = translated.sections.front();
    REQUIRE(markup::render(markup::xml_generator(false), *section) == R"(<details-section>
<paragraph>A. A.</paragraph>
<paragraph>B.</paragraph>
<paragraph>C. C.</paragraph>
</details-section>
)");
}
