// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/code_block.hpp>

#include <catch.hpp>

using namespace standardese::markup;

template <class Entity>
void test_code_block_entity(const char* classes)
{
    auto a = Entity::build("foo");
    REQUIRE(as_html(*a) == R"(<span class=")" + std::string(classes) + R"(">foo</span>)");

    auto b = Entity::build("<foo>");
    REQUIRE(as_html(*b) == R"(<span class=")" + std::string(classes) + R"(">&lt;foo&gt;</span>)");
}

TEST_CASE("code_block::keyword", "[markup]")
{
    test_code_block_entity<code_block::keyword>("kwd");
}

TEST_CASE("code_block::identifier", "[markup]")
{
    test_code_block_entity<code_block::identifier>("typ dec var fun");
}

TEST_CASE("code_block::string_literal", "[markup]")
{
    test_code_block_entity<code_block::string_literal>("str");
}

TEST_CASE("code_block::int_literal", "[markup]")
{
    test_code_block_entity<code_block::int_literal>("lit");
}

TEST_CASE("code_block::float_literal", "[markup]")
{
    test_code_block_entity<code_block::float_literal>("lit");
}

TEST_CASE("code_block::punctuation", "[markup]")
{
    test_code_block_entity<code_block::punctuation>("pun");
}

TEST_CASE("code_block::preprocessor", "[markup]")
{
    test_code_block_entity<code_block::preprocessor>("pre");
}

TEST_CASE("code_block", "[markup]")
{
    auto html =
        R"(<pre><code id="standardese-foo" class="standardese-language-cpp"><span class="kwd">template</span> <span class="pun">&lt;</span><span class="kwd">typename</span> <span class="typ dec var fun">T</span><span class="pun">&gt;</span>
<span class="kwd">void</span> <span class="typ dec var fun">foo</span><span class="pun">();</span>
</code></pre>
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
    REQUIRE(as_html(*builder.finish()) == html);
}
