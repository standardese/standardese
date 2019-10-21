// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/code_block.hpp>

#include <catch.hpp>

#include <standardese/markup/generator.hpp>

using namespace standardese::markup;

template <class Entity>
void test_code_block_entity(const char* name, const char* classes)
{
    auto a = Entity::build("foo");
    REQUIRE(as_html(*a) == R"(<span class=")" + std::string(classes) + R"(">foo</span>)");
    REQUIRE(as_xml(*a->clone()) == "<" + std::string(name) + ">foo</" + std::string(name) + ">");

    auto b = Entity::build("<foo>");
    REQUIRE(as_html(*b) == R"(<span class=")" + std::string(classes) + R"(">&lt;foo&gt;</span>)");
    REQUIRE(as_xml(*b->clone())
            == "<" + std::string(name) + ">&lt;foo&gt;</" + std::string(name) + ">");
}

TEST_CASE("code-block::keyword", "[markup]")
{
    test_code_block_entity<code_block::keyword>("code-block-keyword", "kwd");
}

TEST_CASE("code-block::identifier", "[markup]")
{
    test_code_block_entity<code_block::identifier>("code-block-identifier", "typ dec var fun");
}

TEST_CASE("code-block::string-literal", "[markup]")
{
    test_code_block_entity<code_block::string_literal>("code-block-string-literal", "str");
}

TEST_CASE("code-block::int-literal", "[markup]")
{
    test_code_block_entity<code_block::int_literal>("code-block-int-literal", "lit");
}

TEST_CASE("code-block::float-literal", "[markup]")
{
    test_code_block_entity<code_block::float_literal>("code-block-float-literal", "lit");
}

TEST_CASE("code-block::punctuation", "[markup]")
{
    test_code_block_entity<code_block::punctuation>("code-block-punctuation", "pun");
}

TEST_CASE("code-block::preprocessor", "[markup]")
{
    test_code_block_entity<code_block::preprocessor>("code-block-preprocessor", "pre");
}

TEST_CASE("code-block", "[markup]")
{
    auto html =
        R"(<pre><code id="standardese-foo" class="standardese-language-cpp"><span class="kwd">template</span> <span class="pun">&lt;</span><span class="kwd">typename</span> <span class="typ dec var fun">T</span><span class="pun">&gt;</span>
<span class="kwd">void</span> <span class="typ dec var fun">foo</span><span class="pun">();</span>
</code></pre>
)";

    auto xml =
        R"(<code-block id="foo" language="cpp"><code-block-keyword>template</code-block-keyword> <code-block-punctuation>&lt;</code-block-punctuation><code-block-keyword>typename</code-block-keyword> <code-block-identifier>T</code-block-identifier><code-block-punctuation>&gt;</code-block-punctuation>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>foo</code-block-identifier><code-block-punctuation>();</code-block-punctuation>
</code-block>
)";

    code_block::builder builder(block_id("foo"), "cpp");
    builder.add_child(code_block::keyword::build("template"));
    builder.add_child(text::build(" "));
    builder.add_child(code_block::punctuation::build("<"));
    builder.add_child(code_block::keyword::build("typename"));
    builder.add_child(text::build(" "));
    builder.add_child(code_block::identifier::build("T"));
    builder.add_child(code_block::punctuation::build(">"));
    builder.add_child(text::build("\n"));
    builder.add_child(code_block::keyword::build("void"));
    builder.add_child(text::build(" "));
    builder.add_child(code_block::identifier::build("foo"));
    builder.add_child(code_block::punctuation::build("();"));
    builder.add_child(text::build("\n"));

    auto ptr = builder.finish();
    REQUIRE(as_html(*ptr) == html);
    REQUIRE(as_xml(*ptr->clone()) == xml);
    REQUIRE(render(markdown_generator(false, "", "md"), *ptr) == R"(``` cpp
template <typename T>
void foo();
```
)");
}
