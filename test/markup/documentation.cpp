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

    file_documentation::builder builder(block_id("file-hpp"), heading::build(block_id(), "A file"),
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

    auto ptr = builder.finish();
    REQUIRE(as_html(*ptr) == html);
    REQUIRE(as_xml(*ptr) == xml);
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
</entity-documentation>
</entity-documentation>
)";

    entity_documentation::builder a(block_id("a"), heading::build(block_id(), "Entity A"),
                                    code_block::build(block_id(), "cpp", "void a();"), "module_a");
    entity_documentation::builder b(block_id("b"), heading::build(block_id(), "Entity B"),
                                    code_block::build(block_id(), "cpp", "void b();"), "module_b");
    b.add_brief(
        brief_section::builder().add_child(text::build("The brief documentation.")).finish());
    a.add_child(b.finish());

    auto ptr = a.finish();
    REQUIRE(as_html(*ptr) == html);
    REQUIRE(as_xml(*ptr) == xml);
}
