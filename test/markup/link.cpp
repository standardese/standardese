// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/link.hpp>

#include <catch.hpp>

#include <standardese/markup/document.hpp>
#include <standardese/markup/generator.hpp>
#include <standardese/markup/paragraph.hpp>

using namespace standardese::markup;

TEST_CASE("external_link", "[markup]")
{
    external_link::builder a(url("http://foonathan.net/"));
    a.add_child(emphasis::build("awesome"));
    a.add_child(text::build(" website"));

    auto a_ptr = a.finish()->clone();
    REQUIRE(as_html(*a_ptr) == "<a href=\"http://foonathan.net/\"><em>awesome</em> website</a>");
    REQUIRE(
        as_xml(*a_ptr)
        == R"(<external-link url="http://foonathan.net/"><emphasis>awesome</emphasis> website</external-link>)");

    external_link::builder b("title\"", url("foo/bar/< &>\n"));
    b.add_child(text::build("with title"));

    auto b_ptr = b.finish()->clone();
    REQUIRE(as_html(*b_ptr)
            == "<a href=\"foo/bar/%3C%20&amp;%3E%0A\" title=\"title&quot;\">with title</a>");
    REQUIRE(as_xml(*b_ptr) == R"(<external-link title="title&quot;" url="foo/bar/&lt; &amp;&gt;
">with title</external-link>)");
}

TEST_CASE("documentation_link", "[markup]")
{
    auto doc1 = [] {
        template_document::builder builder("foo", "doc1.templ");
        builder.add_child(paragraph::builder(block_id("p1")).finish());

        paragraph::builder p2(block_id("p2"));
        p2.add_child(documentation_link::builder("title", block_reference(block_id("p1")))
                         .add_child(text::build("link 1"))
                         .finish());
        builder.add_child(p2.finish());

        return builder.finish();
    }();

    auto doc1_html = R"(<section class="standardese-template-document">
<p id="standardese-p1"></p>
<p id="standardese-p2"><a href="#standardese-p1" title="title">link 1</a></p>
</section>
)";
    auto doc1_xml  = R"(<?xml version="1.0" encoding="UTF-8"?>
<template-document output-name="doc1.templ" title="foo">
<paragraph id="p1"></paragraph>
<paragraph id="p2"><documentation-link title="title" destination-id="p1">link 1</documentation-link></paragraph>
</template-document>
)";
    REQUIRE(as_html(*doc1) == doc1_html);
    REQUIRE(as_xml(*doc1->clone()) == doc1_xml);

    documentation_link::builder builder("",
                                        block_reference(output_name::from_file_name("doc1.templ"),
                                                        block_id("p1")));
    builder.add_child(text::build("link 2"));

    auto ptr = builder.finish()->clone();
    REQUIRE(as_html(*ptr) == R"(<a href="doc1.templ#standardese-p1">link 2</a>)");
    REQUIRE(
        as_xml(*ptr)
        == R"(<documentation-link destination-document="doc1.templ" destination-id="p1">link 2</documentation-link>)");

    // non existing link, but doesn't matter
    documentation_link::builder builder2("", block_reference(output_name::from_name("doc2"),
                                                             block_id("p3")));
    builder2.add_child(text::build("link 3"));

    auto ptr2 = builder2.finish()->clone();
    REQUIRE(as_html(*ptr2) == R"(<a href="doc2.html#standardese-p3">link 3</a>)");
    REQUIRE(
        as_xml(*ptr2)
        == R"(<documentation-link destination-document="doc2" destination-id="p3">link 3</documentation-link>)");

    // URL link
    auto ptr3 = documentation_link::builder("").add_child(text::build("link 4")).finish();
    ptr3->resolve_destination(url("http://foonathan.net"));
    REQUIRE(as_html(*ptr3->clone()) == R"(<a href="http://foonathan.net">link 4</a>)");
    REQUIRE(
        as_xml(*ptr3->clone())
        == R"(<documentation-link destination-url="http://foonathan.net">link 4</documentation-link>)");
}
