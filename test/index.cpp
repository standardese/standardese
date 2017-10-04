// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/index.hpp>

#include <catch.hpp>

#include <cppast/cpp_namespace.hpp>

#include <standardese/markup/document.hpp>
#include <standardese/markup/generator.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("entity_index")
{
    auto file = parse_file({}, "entity_index.cpp", R"(
using a = int;
using b = int;

namespace ns1
{
  using b = int;
  using a = int;
}

namespace ns2
{
  using a = int;

  namespace inner
  {
     using c = int;
  }

  using b = int;
}

using c = int;
)");

    auto brief_doc = markup::brief_section::builder()
                         .add_child(markup::text::build("some brief documentation"))
                         .finish();

    entity_index index;
    cppast::visit(*file, [&](const cppast::cpp_entity& e, cppast::visitor_info info) {
        if (e.kind() == cppast::cpp_file::kind()
            || info.event == cppast::visitor_info::container_entity_exit)
            return true;
        else if (e.kind() == cppast::cpp_namespace::kind())
        {
            auto ns_doc = markup::namespace_documentation::
                builder(type_safe::ref(static_cast<const cppast::cpp_namespace&>(e)),
                        markup::block_id(e.name()),
                        markup::heading::build(markup::block_id(), "no heading"));
            if (e.name() == "ns2")
                ns_doc.add_brief(markup::brief_section::builder()
                                     .add_child(markup::text::build("some brief documentation"))
                                     .finish());
            index.register_namespace(static_cast<const cppast::cpp_namespace&>(e),
                                     std::move(ns_doc));
        }
        else if (e.name() == "b")
            index.register_entity(e.name(), e, type_safe::ref(*brief_doc));
        else
            index.register_entity(e.name(), e, nullptr);
        return true;
    });

    auto xml = R"(<entity-index id="entity-index">
<heading>Project index</heading>
<entity-index-item id="a">
<entity><internal-link unresolved-destination-id="a"><code>a</code></internal-link></entity>
</entity-index-item>
<entity-index-item id="b">
<entity><internal-link unresolved-destination-id="b"><code>b</code></internal-link></entity>
<brief>some brief documentation</brief>
</entity-index-item>
<entity-index-item id="c">
<entity><internal-link unresolved-destination-id="c"><code>c</code></internal-link></entity>
</entity-index-item>
<namespace-documentation id="ns1">
<heading>no heading</heading>
<entity-index-item id="a">
<entity><internal-link unresolved-destination-id="a"><code>a</code></internal-link></entity>
</entity-index-item>
<entity-index-item id="b">
<entity><internal-link unresolved-destination-id="b"><code>b</code></internal-link></entity>
<brief>some brief documentation</brief>
</entity-index-item>
</namespace-documentation>
<namespace-documentation id="ns2">
<heading>no heading</heading>
<brief-section>some brief documentation</brief-section>
<entity-index-item id="a">
<entity><internal-link unresolved-destination-id="a"><code>a</code></internal-link></entity>
</entity-index-item>
<entity-index-item id="b">
<entity><internal-link unresolved-destination-id="b"><code>b</code></internal-link></entity>
<brief>some brief documentation</brief>
</entity-index-item>
<namespace-documentation id="inner">
<heading>no heading</heading>
<entity-index-item id="c">
<entity><internal-link unresolved-destination-id="c"><code>c</code></internal-link></entity>
</entity-index-item>
</namespace-documentation>
</namespace-documentation>
</entity-index>
)";
    REQUIRE(markup::as_xml(*index.generate()) == xml);
}

TEST_CASE("file_index")
{
    auto brief_doc = markup::brief_section::builder()
                         .add_child(markup::text::build("some brief documentation"))
                         .finish();

    auto file_a = cppast::cpp_file::builder("a.cpp").finish({});
    auto file_b = cppast::cpp_file::builder("b.cpp").finish({});
    auto file_c = cppast::cpp_file::builder("c.cpp").finish({});

    file_index index;
    index.register_file(file_c->name(), file_c->name(), nullptr);
    index.register_file(file_a->name(), file_a->name(), nullptr);
    index.register_file(file_b->name(), file_b->name(), type_safe::ref(*brief_doc));

    auto xml = R"(<file-index id="file-index">
<heading>Project files</heading>
<entity-index-item id="a.cpp">
<entity><internal-link unresolved-destination-id="a.cpp"><code>a.cpp</code></internal-link></entity>
</entity-index-item>
<entity-index-item id="b.cpp">
<entity><internal-link unresolved-destination-id="b.cpp"><code>b.cpp</code></internal-link></entity>
<brief>some brief documentation</brief>
</entity-index-item>
<entity-index-item id="c.cpp">
<entity><internal-link unresolved-destination-id="c.cpp"><code>c.cpp</code></internal-link></entity>
</entity-index-item>
</file-index>
)";
    REQUIRE(markup::as_xml(*index.generate()) == xml);
}
