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
    parser p;
    auto   translated = translate_ast(p, read_ast(p, comment));
    REQUIRE(translated.sections.size() == 1u);
    return std::move(translated.sections.front());
}

TEST_CASE("cmark inlines", "[comment]")
{
    auto comment = R"(ignore brief

text
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

TEST_CASE("cmark link", "[comment]")
{
    auto comment = R"(ignore brief

[external link](http://foonathan.net)
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

TEST_CASE("cmark block quote", "[comment]")
{
    auto comment = R"(> Hello World.
>
> Hello World 2.

> A different quote.
> But still the same.)";

    auto xml = R"(<details-section>
<block-quote>
<paragraph>Hello World.</paragraph>
<paragraph>Hello World 2.</paragraph>
</block-quote>
<block-quote>
<paragraph>A different quote.<soft-break></soft-break>
But still the same.</paragraph>
</block-quote>
</details-section>
)";

    auto section = parse_details(comment);
    REQUIRE(markup::as_xml(*section) == xml);
}

TEST_CASE("cmark list", "[comment]")
{
    auto comment = R"(* This list.
* is tight.

List break.

* An item with a paragraph.

  And another paragraph.

* And a different item.

List break.

1. An
2. ordered
3. list

List break.

* A list

* with another
  1. list
  2. inside

* *great*
)";

    auto xml = R"(<details-section>
<unordered-list>
<list-item>
<paragraph>This list.</paragraph>
</list-item>
<list-item>
<paragraph>is tight.</paragraph>
</list-item>
</unordered-list>
<paragraph>List break.</paragraph>
<unordered-list>
<list-item>
<paragraph>An item with a paragraph.</paragraph>
<paragraph>And another paragraph.</paragraph>
</list-item>
<list-item>
<paragraph>And a different item.</paragraph>
</list-item>
</unordered-list>
<paragraph>List break.</paragraph>
<ordered-list>
<list-item>
<paragraph>An</paragraph>
</list-item>
<list-item>
<paragraph>ordered</paragraph>
</list-item>
<list-item>
<paragraph>list</paragraph>
</list-item>
</ordered-list>
<paragraph>List break.</paragraph>
<unordered-list>
<list-item>
<paragraph>A list</paragraph>
</list-item>
<list-item>
<paragraph>with another</paragraph>
<ordered-list>
<list-item>
<paragraph>list</paragraph>
</list-item>
<list-item>
<paragraph>inside</paragraph>
</list-item>
</ordered-list>
</list-item>
<list-item>
<paragraph><emphasis>great</emphasis></paragraph>
</list-item>
</unordered-list>
</details-section>
)";

    auto section = parse_details(comment);
    REQUIRE(markup::as_xml(*section) == xml);
}

TEST_CASE("cmark code block", "[comment]")
{
    auto comment = R"(```
A code block.
```

```cpp
A code block with info.
```
)";

    auto xml = R"(<details-section>
<code-block>A code block.
</code-block>
<code-block language="cpp">A code block with info.
</code-block>
</details-section>
)";

    auto section = parse_details(comment);
    REQUIRE(markup::as_xml(*section) == xml);
}

TEST_CASE("cmark heading", "[comment]")
{
    auto comment = R"(# A

## B

### C

DDD
===

EEE
---
)";

    auto xml = R"(<details-section>
<heading>A</heading>
<subheading>B</subheading>
<subheading>C</subheading>
<heading>DDD</heading>
<subheading>EEE</subheading>
</details-section>
)";

    auto section = parse_details(comment);
    REQUIRE(markup::as_xml(*section) == xml);
}

TEST_CASE("cmark thematic break", "[comment]")
{
    auto comment = R"(ignore brief

A paragraph.

---

A completely different paragraph.
)";

    auto xml = R"(<details-section>
<paragraph>A paragraph.</paragraph>
<thematic-break></thematic-break>
<paragraph>A completely different paragraph.</paragraph>
</details-section>
)";

    auto section = parse_details(comment);
    REQUIRE(markup::as_xml(*section) == xml);
}

TEST_CASE("cmark paragraph", "[comment]")
{
    auto comment = R"(A.
A.

B.

C.
C.)";

    auto xml = R"(<details-section>
<paragraph>A.<soft-break></soft-break>
A.</paragraph>
<paragraph>B.</paragraph>
<paragraph>C.<soft-break></soft-break>
C.</paragraph>
</details-section>
)";

    auto section = parse_details(comment);
    REQUIRE(markup::as_xml(*section) == xml);
}

TEST_CASE("sections", "[comment]")
{
    const char* comment = nullptr;
    const char* xml     = nullptr;

    SECTION("implicit")
    {
        comment = R"(Implicit brief.

Implicit details.
Still details.

> Also in quote.

```
Or code.
```

* Or
* List
)";

        xml = R"(<brief-section>Implicit brief.</brief-section>
<details-section>
<paragraph>Implicit details.<soft-break></soft-break>
Still details.</paragraph>
<block-quote>
<paragraph>Also in quote.</paragraph>
</block-quote>
<code-block>Or code.
</code-block>
<unordered-list>
<list-item>
<paragraph>Or</paragraph>
</list-item>
<list-item>
<paragraph>List</paragraph>
</list-item>
</unordered-list>
</details-section>
)";
    }
    SECTION("explicit")
    {
        comment = R"(\brief Explicit brief.
Still explicit brief.

\details Explicit details.

Still details.

\effects Explicit effects.
Still effects.

Details again.

\returns Explicit returns.
\notes Explicit notes.
)";

        xml = R"(<brief-section>Explicit brief.<soft-break></soft-break>
Still explicit brief.</brief-section>
<details-section>
<paragraph>Explicit details.</paragraph>
<paragraph>Still details.</paragraph>
</details-section>
<inline-section>Explicit effects.<soft-break></soft-break>
Still effects.</inline-section>
<details-section>
<paragraph>Details again.</paragraph>
</details-section>
<inline-section>Explicit returns.</inline-section>
<inline-section>Explicit notes.</inline-section>
)";
    }
    SECTION("ignored commands")
    {
        comment = R"(Ignore \effects not starting at beginning.
Prevent brief.
\exclude Ignore all lines starting with a command.
But please include me.
\unknown Ignore unknown commands.

> \effects In block quote.

* \effects In list.
)";

        xml = R"(<details-section>
<paragraph>Ignore \effects not starting at beginning.<soft-break></soft-break>
Prevent brief.</paragraph>
<paragraph>But please include me.<soft-break></soft-break>
\unknown Ignore unknown commands.</paragraph>
<block-quote>
<paragraph>\effects In block quote.</paragraph>
</block-quote>
<unordered-list>
<list-item>
<paragraph>\effects In list.</paragraph>
</list-item>
</unordered-list>
</details-section>
)";
    }

    parser p;
    auto   translated = translate_ast(p, read_ast(p, comment));

    auto result = translated.brief ? markup::as_xml(*translated.brief) : "";
    for (auto& section : translated.sections)
        result += markup::as_xml(*section);
    REQUIRE(result == xml);
}
