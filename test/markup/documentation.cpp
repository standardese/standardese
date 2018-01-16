// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/documentation.hpp>

#include <catch.hpp>

#include <standardese/markup/code_block.hpp>
#include <standardese/markup/document.hpp>
#include <standardese/markup/generator.hpp>
#include <standardese/markup/heading.hpp>
#include <standardese/markup/paragraph.hpp>
#include <standardese/markup/list.hpp>
#include <cppast/cpp_file.hpp>
#include <cppast/cpp_namespace.hpp>

using namespace standardese::markup;

TEST_CASE("file_documentation", "[markup]")
{
    auto html = R"(<article id="standardese-file-hpp" class="standardese-file-documentation">
<h1 class="standardese-file-documentation-heading">A file</h1>
<pre><code class="standardese-language-cpp standardese-entity-synopsis">the synopsis();</code></pre>
<p id="standardese-file-hpp-brief" class="standardese-brief-section">The brief documentation.</p>
<dl id="standardese-file-hpp-inline-sections" class="standardese-inline-sections">
<dt>Effects:</dt>
<dd>The effects of the - eh - file.</dd>
<dt>Notes:</dt>
<dd>Some notes.</dd>
</dl>
<p>The details documentation.</p>
<h4 class="standardese-list-section-heading">Return values</h4>
<ul class="standardese-list-section">
<li>
<p>Any integer</p>
</li>
<li>
<dl class="standardese-term-description-item">
<dt>42</dt>
<dd>&mdash; the answer!</dd>
</dl>
</li>
</ul>
</article>
)";

    auto xml = R"(<file-documentation id="file-hpp">
<heading>A file</heading>
<code-block language="cpp">the synopsis();</code-block>
<brief-section id="file-hpp-brief">The brief documentation.</brief-section>
<inline-section name="Effects">The effects of the - eh - file.</inline-section>
<inline-section name="Notes">Some notes.</inline-section>
<details-section>
<paragraph>The details documentation.</paragraph>
</details-section>
<list-section name="Return values">
<list-item>
<paragraph>Any integer</paragraph>
</list-item>
<term-description-item>
<term>42</term>
<description>the answer!</description>
</term-description-item>
</list-section>
</file-documentation>
)";
    auto md  = R"(# A file

<a id="standardese-file-hpp" style="display: none"></a>

<pre><code class="standardese-language-cpp">the synopsis();</code></pre>

The brief documentation.

*Effects:* The effects of the - eh - file.

*Notes:* Some notes.

The details documentation.

#### Return values

  - Any integer
  - 42 &mdash; the answer\!
)";

    cppast::cpp_file::builder file("foo");

    file_documentation::builder builder(type_safe::ref(file.get()), block_id("file-hpp"),
                                        heading::build(block_id(), "A file"),
                                        code_block::build(block_id(), "cpp", "the synopsis();"));
    builder.add_brief(
        brief_section::builder().add_child(text::build("The brief documentation.")).finish());
    builder.add_section(inline_section::builder(section_type::effects, "Effects")
                            .add_child(text::build("The effects of the - eh - file."))
                            .finish());
    builder.add_section(inline_section::builder(section_type::notes, "Notes")
                            .add_child(text::build("Some notes."))
                            .finish());
    builder.add_details(
        details_section::builder()
            .add_child(
                paragraph::builder().add_child(text::build("The details documentation.")).finish())
            .finish());

    unordered_list::builder list{block_id()};
    list.add_item(
        list_item::build(paragraph::builder().add_child(text::build("Any integer")).finish()));
    list.add_item(term_description_item::build(block_id(), term::build(text::build("42")),
                                               description::build(text::build("the answer!"))));
    builder.add_section(list_section::build(section_type::returns, "Return values", list.finish()));

    auto ptr = builder.finish()->clone();
    REQUIRE(as_html(*ptr) == html);
    REQUIRE(as_xml(*ptr) == xml);
    REQUIRE(as_markdown(*ptr) == md);
}

TEST_CASE("entity_documentation", "[markup]")
{
    auto html = R"(<section id="standardese-a" class="standardese-entity-documentation">
<h2 class="standardese-entity-documentation-heading">Entity A<span class="standardese-module">[module_a]</span></h2>
<pre><code class="standardese-language-cpp standardese-entity-synopsis">void a();</code></pre>
<section id="standardese-b" class="standardese-entity-documentation">
<h3 class="standardese-entity-documentation-heading">Entity B<span class="standardese-module">[module_b]</span></h3>
<pre><code class="standardese-language-cpp standardese-entity-synopsis">void b();</code></pre>
<p id="standardese-b-brief" class="standardese-brief-section">The brief documentation.</p>
<p>The details documentation.</p>
</section>
<hr class="standardese-entity-documentation-break" />
</section>
<hr class="standardese-entity-documentation-break" />
)";
    auto xml  = R"(<entity-documentation id="a" module="module_a">
<heading>Entity A</heading>
<code-block language="cpp">void a();</code-block>
<entity-documentation id="b" module="module_b">
<heading>Entity B</heading>
<code-block language="cpp">void b();</code-block>
<brief-section id="b-brief">The brief documentation.</brief-section>
<details-section>
<paragraph>The details documentation.</paragraph>
</details-section>
</entity-documentation>
</entity-documentation>
)";
    auto md   = R"(## Entity A \[module\_a\]

<a id="standardese-a" style="display: none"></a>

<pre><code class="standardese-language-cpp">void a();</code></pre>

### Entity B \[module\_b\]

<a id="standardese-b" style="display: none"></a>

<pre><code class="standardese-language-cpp">void b();</code></pre>

The brief documentation.

The details documentation.

-----

-----
)";

    cppast::cpp_namespace::builder entity("foo", false, false);

    entity_documentation::builder a(type_safe::ref(entity.get()), block_id("a"),
                                    documentation_header(heading::build(block_id(), "Entity A"),
                                                         "module_a"),
                                    code_block::build(block_id(), "cpp", "void a();"));
    entity_documentation::builder b(type_safe::ref(entity.get()), block_id("b"),
                                    documentation_header(heading::build(block_id(), "Entity B"),
                                                         "module_b"),
                                    code_block::build(block_id(), "cpp", "void b();"));
    b.add_brief(
        brief_section::builder().add_child(text::build("The brief documentation.")).finish());
    b.add_details(
        details_section::builder()
            .add_child(
                paragraph::builder().add_child(text::build("The details documentation.")).finish())
            .finish());
    a.add_child(b.finish());

    auto ptr = a.finish()->clone();
    REQUIRE(as_html(*ptr) == html);
    REQUIRE(as_xml(*ptr) == xml);
    REQUIRE(as_markdown(*ptr) == md);
}
