// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/index.hpp>

#include <catch.hpp>

#include <cppast/cpp_namespace.hpp>
#include <standardese/markup/generator.hpp>

using namespace standardese::markup;

std::string remove_trailing_ws(const std::string& string)
{
    std::string result;
    for (auto c : string)
    {
        if (c == '\n')
        {
            while (!result.empty() && result.back() == ' ')
                result.pop_back();
            result.push_back('\n');
        }
        else
            result.push_back(c);
    }
    return result;
}

TEST_CASE("markup::file_index", "[markup]")
{
    file_index::builder b(heading::build(block_id(), "The file index"));
    b.add_child(entity_index_item::build(block_id("a.hpp"), term::build(text::build("a.hpp"))));
    b.add_child(entity_index_item::build(block_id("b.hpp"), term::build(text::build("b.hpp")),
                                         description::build(text::build("with brief"))));
    auto index = b.finish();

    auto xml  = R"(<file-index id="file-index">
<heading>The file index</heading>
<entity-index-item id="a-hpp">
<entity>a.hpp</entity>
</entity-index-item>
<entity-index-item id="b-hpp">
<entity>b.hpp</entity>
<brief>with brief</brief>
</entity-index-item>
</file-index>
)";
    auto html = R"(<ul id="standardese-file-index" class="standardese-file-index">
<h1>The file index</h1>
<li id="standardese-a-hpp" class="standardese-entity-index-item">
<dl>
<dt>a.hpp</dt>
</dl>
</li>
<li id="standardese-b-hpp" class="standardese-entity-index-item">
<dl>
<dt>b.hpp</dt>
<dd>&mdash; with brief</dd>
</dl>
</li>
</ul>
)";
    auto md   = R"(# The file index

  - a.hpp

  - b.hpp &mdash; with brief
)";

    REQUIRE(as_xml(*index->clone()) == xml);
    REQUIRE(as_html(*index) == html);
    REQUIRE(as_markdown(*index) == md);
}

TEST_CASE("markup::entity_index", "[markup]")
{
    cppast::cpp_namespace::builder ns("foo", false, false);

    // note: no need to test entity_index_item, already done by file
    entity_index::builder b(heading::build(block_id(), "The entity index"));

    namespace_documentation::builder ns1(type_safe::ref(ns.get()), block_id("ns1"),
                                         documentation_header(
                                             heading::build(block_id(), "Namespace ns1")));
    ns1.add_child(entity_index_item::build(block_id("a"), term::build(text::build("Entity a"))));
    b.add_child(ns1.finish());

    namespace_documentation::builder ns2(type_safe::ref(ns.get()), block_id("ns2"),
                                         documentation_header(heading::build(block_id(),
                                                                             "Namespace ns2"),
                                                              "module"));
    ns2.add_brief(brief_section::builder().add_child(text::build("Brief documentation")).finish());
    ns2.add_details(details_section::builder()
                        .add_child(paragraph::builder().add_child(text::build("Details")).finish())
                        .finish());
    namespace_documentation::builder ns3(type_safe::ref(ns.get()), block_id("ns3"),
                                         heading::build(block_id(), "Namespace ns3"));
    ns3.add_brief(brief_section::builder().add_child(text::build("Brief")).finish());
    ns2.add_child(ns3.finish());
    ns2.add_child(entity_index_item::build(block_id("b"), term::build(text::build("Entity b"))));
    b.add_child(ns2.finish());

    auto index = b.finish();

    auto xml  = R"(<entity-index id="entity-index">
<heading>The entity index</heading>
<namespace-documentation id="ns1">
<heading>Namespace ns1</heading>
<entity-index-item id="a">
<entity>Entity a</entity>
</entity-index-item>
</namespace-documentation>
<namespace-documentation id="ns2" module="module">
<heading>Namespace ns2</heading>
<brief-section>Brief documentation</brief-section>
<details-section>
<paragraph>Details</paragraph>
</details-section>
<namespace-documentation id="ns3">
<heading>Namespace ns3</heading>
<brief-section>Brief</brief-section>
</namespace-documentation>
<entity-index-item id="b">
<entity>Entity b</entity>
</entity-index-item>
</namespace-documentation>
</entity-index>
)";
    auto html = R"(<ul id="standardese-entity-index" class="standardese-entity-index">
<h1>The entity index</h1>
<li id="standardese-ns1" class="standardese-namespace-documentation">
<h2 class="standardese-namespace-documentation-heading">Namespace ns1</h2>
<ul class="standardese-namespace-members">
<li id="standardese-a" class="standardese-entity-index-item">
<dl>
<dt>Entity a</dt>
</dl>
</li>
</ul>
</li>
<li id="standardese-ns2" class="standardese-namespace-documentation">
<h2 class="standardese-namespace-documentation-heading">Namespace ns2<span class="standardese-module">[module]</span></h2>
<p class="standardese-brief-section">Brief documentation</p>
<p>Details</p>
<ul class="standardese-namespace-members">
<li id="standardese-ns3" class="standardese-namespace-documentation">
<h3 class="standardese-namespace-documentation-heading">Namespace ns3</h3>
<p class="standardese-brief-section">Brief</p>
<ul class="standardese-namespace-members">
</ul>
</li>
<li id="standardese-b" class="standardese-entity-index-item">
<dl>
<dt>Entity b</dt>
</dl>
</li>
</ul>
</li>
</ul>
)";
    auto md   = R"(# The entity index

  - ## Namespace ns1

    <span id="standardese-ns1"></span>

      - Entity a

  - ## Namespace ns2 \[module\]

    <span id="standardese-ns2"></span>

    Brief documentation

    Details

      - ### Namespace ns3

        <span id="standardese-ns3"></span>

        Brief

      - Entity b
)";

    REQUIRE(as_xml(*index->clone()) == xml);
    REQUIRE(as_html(*index) == html);
    REQUIRE(remove_trailing_ws(as_markdown(*index)) == md);
}

TEST_CASE("markup::module_index", "[markup]")
{
    // note: no need to test entity_index_item, already done by file
    module_index::builder b(heading::build(block_id(), "The module index"));

    module_documentation::builder module1(block_id("module1"),
                                          heading::build(block_id(), "Module 1"));
    module1.add_child(
        entity_index_item::build(block_id("a"), term::build(text::build("Entity a"))));
    b.add_child(module1.finish());

    module_documentation::builder module2(block_id("module2"),
                                          heading::build(block_id(), "Module 2"));
    module2.add_brief(brief_section::builder().add_child(text::build("Brief")).finish());
    module2.add_details(
        details_section::builder()
            .add_child(paragraph::builder().add_child(text::build("Details")).finish())
            .finish());
    module2.add_child(
        entity_index_item::build(block_id("b"), term::build(text::build("Entity b"))));
    b.add_child(module2.finish());

    auto index = b.finish();

    auto xml  = R"(<module-index id="module-index">
<heading>The module index</heading>
<module-documentation id="module1">
<heading>Module 1</heading>
<entity-index-item id="a">
<entity>Entity a</entity>
</entity-index-item>
</module-documentation>
<module-documentation id="module2">
<heading>Module 2</heading>
<brief-section>Brief</brief-section>
<details-section>
<paragraph>Details</paragraph>
</details-section>
<entity-index-item id="b">
<entity>Entity b</entity>
</entity-index-item>
</module-documentation>
</module-index>
)";
    auto html = R"(<ul id="standardese-module-index" class="standardese-module-index">
<h1>The module index</h1>
<li id="standardese-module1" class="standardese-module-documentation">
<h2 class="standardese-module-documentation-heading">Module 1</h2>
<ul class="standardese-module-members">
<li id="standardese-a" class="standardese-entity-index-item">
<dl>
<dt>Entity a</dt>
</dl>
</li>
</ul>
</li>
<li id="standardese-module2" class="standardese-module-documentation">
<h2 class="standardese-module-documentation-heading">Module 2</h2>
<p class="standardese-brief-section">Brief</p>
<p>Details</p>
<ul class="standardese-module-members">
<li id="standardese-b" class="standardese-entity-index-item">
<dl>
<dt>Entity b</dt>
</dl>
</li>
</ul>
</li>
</ul>
)";
    auto md   = R"(# The module index

  - ## Module 1

    <span id="standardese-module1"></span>

      - Entity a

  - ## Module 2

    <span id="standardese-module2"></span>

    Brief

    Details

      - Entity b
)";

    REQUIRE(as_xml(*index->clone()) == xml);
    REQUIRE(as_html(*index) == html);
    REQUIRE(remove_trailing_ws(as_markdown(*index)) == md);
}
