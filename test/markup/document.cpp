// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/document.hpp>

#include <catch.hpp>

#include <standardese/markup/generator.hpp>
#include <standardese/markup/paragraph.hpp>

using namespace standardese::markup;

template <typename T>
void test_main_sub_document(const char* name)
{
    auto html = R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Hello World!</title>
</head>
<body>
<p>foo</p>
</body>
</html>
)";

    auto xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<)" + std::string(name)
               + R"( output-name="my-file" title="Hello World!">
<paragraph>foo</paragraph>
</)" + name + ">\n";

    typename T::builder builder("Hello World!", "my-file");
    builder.add_child(paragraph::builder(block_id("")).add_child(text::build("foo")).finish());

    auto doc = builder.finish();
    REQUIRE(doc->output_name().name() == "my-file");
    REQUIRE(doc->output_name().needs_extension());
    REQUIRE(as_html(*doc) == html);
    REQUIRE(as_xml(*doc) == xml);
}

TEST_CASE("main_document", "[markup]")
{
    test_main_sub_document<main_document>("main-document");
}

TEST_CASE("subdocument", "[markup]")
{
    test_main_sub_document<subdocument>("subdocument");
}

TEST_CASE("template_document", "[markup]")
{
    auto html = R"(<section class="standardese-template-document">
<p></p>
</section>
)";

    auto xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<template-document output-name="foo.bar.baz" title="Hello Templated World!">
<paragraph></paragraph>
</template-document>
)";

    template_document::builder builder("Hello Templated World!", "foo.bar.baz");
    builder.add_child(paragraph::builder(block_id("")).finish());

    auto doc = builder.finish();
    REQUIRE(doc->output_name().name() == "foo.bar.baz");
    REQUIRE(!doc->output_name().needs_extension());
    REQUIRE(as_html(*doc) == html);
    REQUIRE(as_xml(*doc) == xml);
}
