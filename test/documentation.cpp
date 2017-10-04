// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/doc_entity.hpp>

#include <catch.hpp>

#include <standardese/markup/document.hpp>
#include <standardese/markup/generator.hpp>

#include "test_parser.hpp"

using namespace standardese;

// no need to test documentation comments extensively
// just the top-level documentation hierarchy
TEST_CASE("documentation")
{
    comment_registry         comments;
    cppast::cpp_entity_index index;

    SECTION("basic")
    {
        auto file = build_doc_entities(comments, index, "documentation__basic.cpp", R"(
/// A function.
/// \effects Effects.
void foo();

namespace ns
{
    /// A class.
    class bar
    {
        int exclude_me;

        /// A virtual function.
        virtual void f1(int i);

    public:
       /// A member function.
       /// \module module
       void f2() const {}
    };
}
)");

        auto doc = generate_documentation({}, {}, index, *file);
        REQUIRE(markup::as_xml(*doc) == R"*(<file-documentation id="documentation__basic.cpp">
<heading>Header file <code>documentation__basic.cpp</code></heading>
<code-block language="cpp"><code-block-keyword>void</code-block-keyword> <internal-link unresolved-destination-id="foo()"><code-block-identifier>foo</code-block-identifier></internal-link><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>namespace</code-block-keyword> <code-block-identifier>ns</code-block-identifier><soft-break></soft-break>
<code-block-punctuation>{</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>class</code-block-keyword> <internal-link unresolved-destination-id="ns::bar"><code-block-identifier>bar</code-block-identifier></internal-link><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-punctuation>}</code-block-punctuation><soft-break></soft-break>
</code-block>
<entity-documentation id="foo()">
<heading>Function <code>foo</code></heading>
<code-block language="cpp"><code-block-keyword>void</code-block-keyword> <code-block-identifier>foo</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
<brief-section id="foo()-brief">A function.</brief-section>
<inline-section name="Effects">Effects.</inline-section>
</entity-documentation>
<entity-documentation id="ns">
<entity-documentation id="ns::bar">
<heading>Class <code>bar</code></heading>
<code-block language="cpp"><code-block-keyword>class</code-block-keyword> <code-block-identifier>bar</code-block-identifier><soft-break></soft-break>
<code-block-punctuation>{</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>virtual</code-block-keyword> <code-block-keyword>void</code-block-keyword> <internal-link unresolved-destination-id="ns::bar::f1(int)"><code-block-identifier>f1</code-block-identifier></internal-link><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword> <code-block-identifier>i</code-block-identifier><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>public</code-block-keyword><code-block-punctuation>:</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>void</code-block-keyword> <internal-link unresolved-destination-id="ns::bar::f2()const"><code-block-identifier>f2</code-block-identifier></internal-link><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation> <code-block-keyword>const</code-block-keyword><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-punctuation>};</code-block-punctuation><soft-break></soft-break>
</code-block>
<brief-section id="ns::bar-brief">A class.</brief-section>
<entity-documentation id="ns::bar::f1(int)">
<heading>Function <code>f1</code></heading>
<code-block language="cpp"><code-block-keyword>virtual</code-block-keyword> <code-block-keyword>void</code-block-keyword> <code-block-identifier>f1</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword> <code-block-identifier>i</code-block-identifier><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
<brief-section id="ns::bar::f1(int)-brief">A virtual function.</brief-section>
</entity-documentation>
<entity-documentation id="ns::bar::f2()const" module="module">
<heading>Function <code>f2</code></heading>
<code-block language="cpp"><code-block-keyword>void</code-block-keyword> <code-block-identifier>f2</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation> <code-block-keyword>const</code-block-keyword><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
<brief-section id="ns::bar::f2()const-brief">A member function.</brief-section>
</entity-documentation>
</entity-documentation>
</entity-documentation>
</file-documentation>
)*");
    }
    SECTION("inlines")
    {
        auto file = build_doc_entities(comments, index, "documentation__inlines.cpp", R"(
/// Class.
/// \tparam A A
/// \tparam B B
/// \base C C
template <typename A, typename B, typename C>
struct foo : C
{
    int d; //< d
    float e;
    void* f; //< f
};

/// Function.
/// \param a a
/// \param b b
void func(int a, int b);

/// Enum.
enum class bar
{
    a, //< a
    b, //< b
};
)");

        auto doc = generate_documentation({}, {}, index, *file);
        REQUIRE(markup::as_xml(*doc) == R"*(<file-documentation id="documentation__inlines.cpp">
<heading>Header file <code>documentation__inlines.cpp</code></heading>
<code-block language="cpp"><code-block-keyword>template</code-block-keyword> <code-block-punctuation>&lt;</code-block-punctuation><code-block-keyword>typename</code-block-keyword> <internal-link unresolved-destination-id="foo&lt;A,B,C&gt;.A"><code-block-identifier>A</code-block-identifier></internal-link><code-block-punctuation>,</code-block-punctuation> <code-block-keyword>typename</code-block-keyword> <internal-link unresolved-destination-id="foo&lt;A,B,C&gt;.B"><code-block-identifier>B</code-block-identifier></internal-link><code-block-punctuation>,</code-block-punctuation> <code-block-keyword>typename</code-block-keyword> <code-block-identifier>C</code-block-identifier><code-block-punctuation>&gt;</code-block-punctuation><soft-break></soft-break>
<code-block-keyword>struct</code-block-keyword> <internal-link unresolved-destination-id="foo&lt;A,B,C&gt;"><code-block-identifier>foo</code-block-identifier></internal-link><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <internal-link unresolved-destination-id="func(int,int)"><code-block-identifier>func</code-block-identifier></internal-link><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword> <internal-link unresolved-destination-id="func(int,int).a"><code-block-identifier>a</code-block-identifier></internal-link><code-block-punctuation>,</code-block-punctuation> <code-block-keyword>int</code-block-keyword> <internal-link unresolved-destination-id="func(int,int).b"><code-block-identifier>b</code-block-identifier></internal-link><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>enum</code-block-keyword> <code-block-keyword>class</code-block-keyword> <internal-link unresolved-destination-id="bar"><code-block-identifier>bar</code-block-identifier></internal-link><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
<entity-documentation id="foo&lt;A,B,C&gt;">
<heading>Struct <code>foo</code></heading>
<code-block language="cpp"><code-block-keyword>template</code-block-keyword> <code-block-punctuation>&lt;</code-block-punctuation><code-block-keyword>typename</code-block-keyword> <internal-link unresolved-destination-id="foo&lt;A,B,C&gt;.A"><code-block-identifier>A</code-block-identifier></internal-link><code-block-punctuation>,</code-block-punctuation> <code-block-keyword>typename</code-block-keyword> <internal-link unresolved-destination-id="foo&lt;A,B,C&gt;.B"><code-block-identifier>B</code-block-identifier></internal-link><code-block-punctuation>,</code-block-punctuation> <code-block-keyword>typename</code-block-keyword> <code-block-identifier>C</code-block-identifier><code-block-punctuation>&gt;</code-block-punctuation><soft-break></soft-break>
<code-block-keyword>struct</code-block-keyword> <code-block-identifier>foo</code-block-identifier><soft-break></soft-break>
<code-block-punctuation>:</code-block-punctuation> <internal-link unresolved-destination-id="foo&lt;A,B,C&gt;::C"><code-block-identifier>C</code-block-identifier></internal-link><soft-break></soft-break>
<code-block-punctuation>{</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>int</code-block-keyword> <internal-link unresolved-destination-id="foo&lt;A,B,C&gt;::d"><code-block-identifier>d</code-block-identifier></internal-link><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
    <code-block-keyword>float</code-block-keyword> <code-block-identifier>e</code-block-identifier><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
    <code-block-keyword>void</code-block-keyword><code-block-punctuation>*</code-block-punctuation> <internal-link unresolved-destination-id="foo&lt;A,B,C&gt;::f"><code-block-identifier>f</code-block-identifier></internal-link><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-punctuation>};</code-block-punctuation><soft-break></soft-break>
</code-block>
<brief-section id="foo&lt;A,B,C&gt;-brief">Class.</brief-section>
<list-section name="Template parameters">
<term-description-item id="foo&lt;A,B,C&gt;.A">
<term><code>A</code></term>
<description>A</description>
</term-description-item>
<term-description-item id="foo&lt;A,B,C&gt;.B">
<term><code>B</code></term>
<description>B</description>
</term-description-item>
</list-section>
<list-section name="Base classes">
<term-description-item id="foo&lt;A,B,C&gt;::C">
<term><code>C</code></term>
<description>C</description>
</term-description-item>
</list-section>
<list-section name="Member variables">
<term-description-item id="foo&lt;A,B,C&gt;::d">
<term><code>d</code></term>
<description>d</description>
</term-description-item>
<term-description-item id="foo&lt;A,B,C&gt;::f">
<term><code>f</code></term>
<description>f</description>
</term-description-item>
</list-section>
</entity-documentation>
<entity-documentation id="func(int,int)">
<heading>Function <code>func</code></heading>
<code-block language="cpp"><code-block-keyword>void</code-block-keyword> <code-block-identifier>func</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword> <internal-link unresolved-destination-id="func(int,int).a"><code-block-identifier>a</code-block-identifier></internal-link><code-block-punctuation>,</code-block-punctuation> <code-block-keyword>int</code-block-keyword> <internal-link unresolved-destination-id="func(int,int).b"><code-block-identifier>b</code-block-identifier></internal-link><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
<brief-section id="func(int,int)-brief">Function.</brief-section>
<list-section name="Parameters">
<term-description-item id="func(int,int).a">
<term><code>a</code></term>
<description>a</description>
</term-description-item>
<term-description-item id="func(int,int).b">
<term><code>b</code></term>
<description>b</description>
</term-description-item>
</list-section>
</entity-documentation>
<entity-documentation id="bar">
<heading>Enumeration <code>bar</code></heading>
<code-block language="cpp"><code-block-keyword>enum</code-block-keyword> <code-block-keyword>class</code-block-keyword> <code-block-identifier>bar</code-block-identifier><soft-break></soft-break>
<code-block-punctuation>{</code-block-punctuation><soft-break></soft-break>
    <internal-link unresolved-destination-id="bar::a"><code-block-identifier>a</code-block-identifier></internal-link><code-block-punctuation>,</code-block-punctuation><soft-break></soft-break>
    <internal-link unresolved-destination-id="bar::b"><code-block-identifier>b</code-block-identifier></internal-link><soft-break></soft-break>
<code-block-punctuation>};</code-block-punctuation><soft-break></soft-break>
</code-block>
<brief-section id="bar-brief">Enum.</brief-section>
<list-section name="Enumerators">
<term-description-item id="bar::a">
<term><code>a</code></term>
<description>a</description>
</term-description-item>
<term-description-item id="bar::b">
<term><code>b</code></term>
<description>b</description>
</term-description-item>
</list-section>
</entity-documentation>
</file-documentation>
)*");
    }
    SECTION("groups")
    {
        auto file = build_doc_entities(comments, index, "documentation__groups.cpp", R"(
/// Documentation.
/// \group a The a
void a();

/// \group a
void a(int param);
)");

        auto doc = generate_documentation({}, {}, index, *file);
        REQUIRE(markup::as_xml(*doc) == R"*(<file-documentation id="documentation__groups.cpp">
<heading>Header file <code>documentation__groups.cpp</code></heading>
<code-block language="cpp">//=== The a ===//<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <internal-link unresolved-destination-id="a"><code-block-identifier>a</code-block-identifier></internal-link><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <internal-link unresolved-destination-id="a(int)"><code-block-identifier>a</code-block-identifier></internal-link><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword> <code-block-identifier>param</code-block-identifier><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
<entity-documentation id="a()">
<heading>The a</heading>
<code-block language="cpp">(1) <code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
(2) <code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword> <code-block-identifier>param</code-block-identifier><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
<brief-section id="a()-brief">Documentation.</brief-section>
</entity-documentation>
</file-documentation>
)*");
    }
}
