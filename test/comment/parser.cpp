// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment/parser.hpp>

#include <catch.hpp>

#include <standardese/markup/generator.hpp>

using namespace standardese;
using namespace standardese::comment;

// returns the details section of the given comment
std::unique_ptr<markup::doc_section> parse_details(const char* comment)
{
    auto translated = translate_ast(read_ast(parser(), comment));
    REQUIRE(translated.sections.size() == 1u);
    return std::move(translated.sections.front());
}

TEST_CASE("cmark inlines", "[comment]")
{
    auto comment = R"(text
`code`
*emphasis with `code`*\
**strong emphasis with _emphasis_**
)";

    auto xml = R"(<details-section>
<paragraph>text<soft-break></soft-break>
<code>code</code><soft-break></soft-break>
<emphasis>emphasis with <code>code</code></emphasis><hard-break></hard-break>
<strong-emphasis>strong emphasis with <emphasis>emphasis</emphasis></strong-emphasis></paragraph>
</details-section>
)";

    auto section = parse_details(comment);
    REQUIRE(markup::as_xml(*section) == xml);
}

TEST_CASE("link", "[comment]")
{
    auto comment = R"([external link](http://foonathan.net)
[external link `2`](http://standardese.foonathan.net/ "title")
[internal link](<> "name")
[internal link `2`](standardese://name/ "title")
[name]()
)";

    auto xml = R"(<details-section>
<paragraph><external-link url="http://foonathan.net">external link</external-link><soft-break></soft-break>
<external-link title="title" url="http://standardese.foonathan.net/">external link <code>2</code></external-link><soft-break></soft-break>
<internal-link destination-id="name">internal link</internal-link><soft-break></soft-break>
<internal-link title="title" destination-id="name">internal link <code>2</code></internal-link><soft-break></soft-break>
<internal-link destination-id="name"></internal-link></paragraph>
</details-section>
)";

    auto section = parse_details(comment);
    REQUIRE(markup::as_xml(*section) == xml);
}

TEST_CASE("forbidden cmark entities", "[comment]")
{
    REQUIRE_THROWS_AS(parse_details(R"(Text <span>inline HTML</span>.)"), translation_error);
    REQUIRE_THROWS_AS(parse_details(R"(<p>block HTML</p>)"), translation_error);
    REQUIRE_THROWS_AS(parse_details(R"(![an image](img.png))"), translation_error);
}

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
<paragraph>A.<soft-break></soft-break>
A.</paragraph>
<paragraph>B.</paragraph>
<paragraph>C.<soft-break></soft-break>
C.</paragraph>
</details-section>
)");
}
