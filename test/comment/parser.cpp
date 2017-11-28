// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment/parser.hpp>

#include <catch.hpp>

#include <standardese/markup/generator.hpp>

using namespace standardese;
using namespace standardese::comment;

void check_details(const char* comment, const char* xml)
{
    parser p;
    auto   result = parse(p, comment, true);
    REQUIRE(result.comment.has_value());
    REQUIRE(result.comment.value().sections().size() == 1u);
    REQUIRE(markup::as_xml(*result.comment.value().sections().begin()) == xml);
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
<emphasis>emphasis with <code>code</code></emphasis></paragraph>
<paragraph><strong-emphasis>strong emphasis with <emphasis>emphasis</emphasis></strong-emphasis></paragraph>
</details-section>
)";

    check_details(comment, xml);
}

TEST_CASE("cmark link", "[comment]")
{
    auto comment = R"(ignore brief

[external link](http://foonathan.net)
[external link `2`](http://standardese.foonathan.net/ "title")
[internal <link>](<> "name")
[internal link `2`](standardese://name/ "title")
[name<T>name]()
)";

    auto xml = R"(<details-section>
<paragraph><external-link url="http://foonathan.net">external link</external-link><soft-break></soft-break>
<external-link title="title" url="http://standardese.foonathan.net/">external link <code>2</code></external-link><soft-break></soft-break>
<documentation-link unresolved-destination-id="name">internal &lt;link&gt;</documentation-link><soft-break></soft-break>
<documentation-link title="title" unresolved-destination-id="name">internal link <code>2</code></documentation-link><soft-break></soft-break>
<documentation-link unresolved-destination-id="name&lt;T&gt;name"><code>name&lt;T&gt;name</code></documentation-link></paragraph>
</details-section>
)";

    check_details(comment, xml);
}

TEST_CASE("forbidden cmark entities", "[comment]")
{
    REQUIRE_THROWS_AS(check_details(R"(<p>block HTML</p>)", ""), parse_error);
    REQUIRE_THROWS_AS(check_details(R"(![an image](img.png))", ""), parse_error);
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

    check_details(comment, xml);
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

    check_details(comment, xml);
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

    check_details(comment, xml);
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

    check_details(comment, xml);
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

    check_details(comment, xml);
}

TEST_CASE("cmark paragraph", "[comment]")
{
    SECTION("basic")
    {
        auto comment = R"(Brief.
A.
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

        check_details(comment, xml);
    }
    SECTION("don't break sentences in brief")
    {
        auto comment = R"(Brief
sentence
split
into
multiple!
Not brief.)";

        auto xml = R"(<details-section>
<paragraph>Not brief.</paragraph>
</details-section>
)";

        check_details(comment, xml);
    }
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
Even still details.

> Also in quote.

```
Or code.
```

* Or
* List
)";

        xml = R"(<brief-section>Implicit brief.</brief-section>
<details-section>
<paragraph>Implicit details.</paragraph>
<paragraph>Still details.<soft-break></soft-break>
Even still details.</paragraph>
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
\returns Different returns.\
Details again.
\notes Explicit notes.
)";

        xml = R"(<brief-section>Explicit brief.<soft-break></soft-break>
Still explicit brief.</brief-section>
<details-section>
<paragraph>Explicit details.</paragraph>
<paragraph>Still details.</paragraph>
</details-section>
<inline-section name="Effects">Explicit effects.<soft-break></soft-break>
Still effects.</inline-section>
<details-section>
<paragraph>Details again.</paragraph>
</details-section>
<inline-section name="Returns">Explicit returns.</inline-section>
<inline-section name="Returns">Different returns.</inline-section>
<details-section>
<paragraph>Details again.</paragraph>
</details-section>
<inline-section name="Notes">Explicit notes.</inline-section>
)";
    }
    SECTION("ignored commands")
    {
        comment = R"(\details Ignore \effects not starting at beginning.
Prevent brief.
\synopsis Ignore all lines starting with a command.
But please include me.

> \effects In block quote.

* \effects In list.
)";

        xml = R"(<details-section>
<paragraph>Ignore \effects not starting at beginning.<soft-break></soft-break>
Prevent brief.</paragraph>
<paragraph>But please include me.</paragraph>
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
    SECTION("key-value sections")
    {
        comment = R"(\returns 0 - Value 0.
\returns 1-Value 1.
It requires extra long description.
\returns Default returns.
\notes This terminates.

\see [foo] - Optional description.
\see [bar]-

This terminates.
)";
        xml     = R"(<list-section name="Return values">
<term-description-item>
<term>0</term>
<description>Value 0.</description>
</term-description-item>
<term-description-item>
<term>1</term>
<description>Value 1.<soft-break></soft-break>
It requires extra long description.</description>
</term-description-item>
<list-item>
<paragraph>Default returns.</paragraph>
</list-item>
</list-section>
<inline-section name="Notes">This terminates.</inline-section>
<list-section name="See also">
<term-description-item>
<term><documentation-link unresolved-destination-id="foo">foo</documentation-link></term>
<description>Optional description.</description>
</term-description-item>
<term-description-item>
<term><documentation-link unresolved-destination-id="bar">bar</documentation-link></term>
<description></description>
</term-description-item>
</list-section>
<details-section>
<paragraph>This terminates.</paragraph>
</details-section>
)";
    }

    parser p;
    auto   parsed = parse(p, comment, true);

    auto result = parsed.comment.value().brief_section() ?
                      markup::as_xml(parsed.comment.value().brief_section().value()) :
                      "";
    for (auto& section : parsed.comment.value().sections())
        result += markup::as_xml(section);
    REQUIRE(result == xml);
}

metadata parse_metadata(const char* comment)
{
    parser p;
    return parse(p, comment, true).comment.value().metadata();
}

matching_entity parse_entity(const char* comment)
{
    parser p;
    return parse(p, comment, false).entity;
}

TEST_CASE("commands", "[comment]")
{
    SECTION("exclude")
    {
        REQUIRE(parse_metadata("foo\nbar").exclude() == type_safe::nullopt);
        REQUIRE(parse_metadata(R"(\exclude)").exclude() == exclude_mode::entity);
        REQUIRE(parse_metadata(R"(\exclude target)").exclude() == exclude_mode::target);
        REQUIRE(parse_metadata(R"(\exclude return)").exclude() == exclude_mode::return_type);
        REQUIRE_THROWS_AS(parse_metadata(R"(\exclude foo)"), parse_error);
        REQUIRE_THROWS_AS(parse_metadata("\\exclude\n\\exclude"), parse_error);
    }
    SECTION("unique_name")
    {
        REQUIRE(parse_metadata("foo\nbar").unique_name() == type_safe::nullopt);
        REQUIRE(parse_metadata(R"(\unique_name new)").unique_name() == "new");
        REQUIRE_THROWS_AS(parse_metadata(R"(\unique_name a b c)"), parse_error);
        REQUIRE_THROWS_AS(parse_metadata("\\unique_name a\n\\unique_name b"), parse_error);
    }
    SECTION("synopsis")
    {
        REQUIRE(parse_metadata("foo\nbar").synopsis() == type_safe::nullopt);
        REQUIRE(parse_metadata(R"(\synopsis new)").synopsis() == "new");
        REQUIRE(parse_metadata(R"(\synopsis a b c)").synopsis() == "a b c");
        REQUIRE_THROWS_AS(parse_metadata("\\synopsis a\n\\synopsis b"), parse_error);
    }
    SECTION("group")
    {
        REQUIRE(parse_metadata("foo\nbar").group() == type_safe::nullopt);
        REQUIRE_THROWS_AS(parse_metadata(R"(\group)"), parse_error);
        REQUIRE_THROWS_AS(parse_metadata("\\group a\n\\group b"), parse_error);
        REQUIRE_THROWS_AS(parse_metadata("\\group a\n\\output_section bar"), parse_error);

        auto a = parse_metadata(R"(\group a)").group().value();
        REQUIRE(a.name() == "a");
        REQUIRE(!a.heading());
        REQUIRE(a.output_section().value() == "a");

        auto b = parse_metadata(R"(\group -b)").group().value();
        REQUIRE(b.name() == "b");
        REQUIRE(!b.heading());
        REQUIRE(!b.output_section());

        auto c = parse_metadata(R"(\group c a heading)").group().value();
        REQUIRE(c.name() == "c");
        REQUIRE(c.heading() == "a heading");
        REQUIRE(c.output_section().value() == "a heading");
    }
    SECTION("module")
    {
        // as metadata
        REQUIRE(parse_metadata("foo\nbar").module() == type_safe::nullopt);
        REQUIRE(parse_metadata(R"(\module new)").module() == "new");
        REQUIRE_THROWS_AS(parse_metadata(R"(\module a b c)"), parse_error);
        REQUIRE_THROWS_AS(parse_metadata("\\module a\n\\module b"), parse_error);

        // as module documentation
        REQUIRE(get_module(parse_entity(R"(\module foo)")) == "foo");
    }
    SECTION("output_section")
    {
        REQUIRE(parse_metadata("foo\nbar").output_section() == type_safe::nullopt);
        REQUIRE(parse_metadata(R"(\output_section new)").output_section() == "new");
        REQUIRE(parse_metadata(R"(\output_section a b c)").output_section() == "a b c");
        REQUIRE_THROWS_AS(parse_metadata("\\output_section a\n\\output_section b"), parse_error);
    }
    SECTION("entity")
    {
        REQUIRE(parse_entity("foo\nbar") == type_safe::nullvar);

        REQUIRE(get_remote_entity(parse_entity(R"(\entity new)")) == "new");
        REQUIRE_THROWS_AS(parse_entity("\\entity a\n\\entity b"), parse_error);
        REQUIRE_THROWS_AS(parse_entity("\\entity a\n\\file"), parse_error);
    }
    SECTION("file")
    {
        REQUIRE(is_file(parse_entity(R"(\file)")));
        REQUIRE_THROWS_AS(parse_metadata(R"(\file a)"), parse_error);
    }
    SECTION("invalid")
    {
        REQUIRE_THROWS_AS(parse_metadata(R"(\foo)"), parse_error);
    }
}

matching_entity parse_entity_inline(const char* comment)
{
    parser p;
    auto   result  = parse(p, comment, true);
    auto&  inlines = result.inlines;
    REQUIRE(inlines.size() == 1u);
    return inlines[0u].entity;
}

TEST_CASE("inlines", "[comment]")
{
    SECTION("parsing")
    {
        auto comment = R"(Brief.

Details.

\param foo Param documentation.
Still param.
\param bar New param.
Still param.

Still details.

\base bar Base.

Details again.
)";

        auto xml = R"(<details-section>
<paragraph>Details.</paragraph>
<paragraph>Still details.</paragraph>
<paragraph>Details again.</paragraph>
</details-section>
)";

        check_details(comment, xml);
    }
    SECTION("missing arguments")
    {
        REQUIRE_THROWS_AS(parse_entity_inline(R"(\param)"), parse_error);
        REQUIRE_THROWS_AS(parse_entity_inline(R"(\tparam)"), parse_error);
        REQUIRE_THROWS_AS(parse_entity_inline(R"(\base)"), parse_error);
    }
    SECTION("matching entity")
    {
        REQUIRE(get_inline_param(parse_entity_inline(R"(\param foo)")) == "foo");
        REQUIRE(get_inline_param(parse_entity_inline(R"(\tparam foo)")) == "foo");
        REQUIRE(get_inline_base(parse_entity_inline(R"(\base foo)")) == "foo");
    }
    SECTION("content")
    {
        // just use param in all examples, doesn't matter
        auto comment = R"(\param a This is brief.
This is details.
\details This is still details.

This is unrelated.

\param b This is just brief.
\effects Section of inline.\
This is unrelated.

\param c This is brief.
This is details.
\exclude
This is details.\
This is unrelated.

\param d
\module d
)";

        parser p;
        auto   result = parse(p, comment, true);

        auto xml = R"(<brief-section>This is unrelated.</brief-section>
<details-section>
<paragraph>This is unrelated.</paragraph>
<paragraph>This is unrelated.</paragraph>
</details-section>
)";
        {
            auto str = result.comment.value().brief_section() ?
                           markup::as_xml(result.comment.value().brief_section().value()) :
                           "";
            for (auto& sec : result.comment.value().sections())
                str += markup::as_xml(sec);
            REQUIRE(str == xml);
        }

        auto& inlines = result.inlines;
        REQUIRE(inlines.size() == 4u);

        auto xml_a = R"(<brief-section>This is brief.</brief-section>
<details-section>
<paragraph>This is details.</paragraph>
<paragraph>This is still details.</paragraph>
</details-section>
)";
        REQUIRE(markup::as_xml(inlines[0].comment.brief_section().value())
                    + markup::as_xml(*inlines[0].comment.sections().begin())
                == xml_a);

        auto xml_b = R"(<brief-section>This is just brief.</brief-section>
<inline-section name="Effects">Section of inline.</inline-section>
)";
        REQUIRE(markup::as_xml(inlines[1].comment.brief_section().value())
                    + markup::as_xml(*inlines[1].comment.sections().begin())
                == xml_b);

        auto xml_c = R"(<brief-section>This is brief.</brief-section>
<details-section>
<paragraph>This is details.</paragraph>
<paragraph>This is details.</paragraph>
</details-section>
)";

        REQUIRE(inlines[2].comment.metadata().exclude());
        REQUIRE(markup::as_xml(inlines[2].comment.brief_section().value())
                    + markup::as_xml(*inlines[2].comment.sections().begin())
                == xml_c);

        REQUIRE(inlines[3].comment.sections().empty());
        REQUIRE(!inlines[3].comment.brief_section());
        REQUIRE(inlines[3].comment.metadata().module() == "d");
    }
}
