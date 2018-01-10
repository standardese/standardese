// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/doc_entity.hpp>

#include <catch.hpp>

#include <standardese/markup/generator.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("synopsis")
{
    standardese::comment_registry comments;
    cppast::cpp_entity_index      index;

    SECTION("basic")
    {
        auto file          = build_doc_entities(comments, index, "synopsis__basic.cpp", R"(
#define FOO hidden

void func(int i, char c = 42);

namespace ns
{
   template <typename T>
   struct foo
   {
       int member;

       foo(const foo& f) = default;

       /// \group do_sth
       void do_sth() const {}

       /// \group do_sth
       void do_sth(int) noexcept;

       /// \group do_sth
       void do_sth(float) &&;

       /// \group do_sth
       void do_sth(float) const volatile && noexcept;
   };
}

ns::foo<ns::foo<int>> make();
)");
        auto file_synopsis = generate_synopsis({}, index, *file);
        REQUIRE(
            markup::as_xml(*file_synopsis)
            == R"(<code-block language="cpp"><code-block-preprocessor>#define</code-block-preprocessor> <code-block-identifier>FOO</code-block-identifier><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>func</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword> <code-block-identifier>i</code-block-identifier><code-block-punctuation>,</code-block-punctuation> <code-block-keyword>char</code-block-keyword> <code-block-identifier>c</code-block-identifier> <code-block-punctuation>=</code-block-punctuation> <code-block-int-literal>42</code-block-int-literal><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>namespace</code-block-keyword> <code-block-identifier>ns</code-block-identifier><soft-break></soft-break>
<code-block-punctuation>{</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>template</code-block-keyword> <code-block-punctuation>&lt;</code-block-punctuation><code-block-keyword>typename</code-block-keyword> <code-block-identifier>T</code-block-identifier><code-block-punctuation>&gt;</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>struct</code-block-keyword> <code-block-identifier>foo</code-block-identifier><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-punctuation>}</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-identifier>ns::foo</code-block-identifier><code-block-punctuation>&lt;</code-block-punctuation>ns::foo&lt;int&gt;<code-block-punctuation>&gt;</code-block-punctuation> <code-block-identifier>make</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
)");

        auto& foo          = get_named_entity(*file, "foo");
        auto  foo_synopsis = generate_synopsis({}, index, foo);
        REQUIRE(
            markup::as_xml(*foo_synopsis)
            == R"*(<code-block language="cpp"><code-block-keyword>template</code-block-keyword> <code-block-punctuation>&lt;</code-block-punctuation><code-block-keyword>typename</code-block-keyword> <code-block-identifier>T</code-block-identifier><code-block-punctuation>&gt;</code-block-punctuation><soft-break></soft-break>
<code-block-keyword>struct</code-block-keyword> <code-block-identifier>foo</code-block-identifier><soft-break></soft-break>
<code-block-punctuation>{</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>int</code-block-keyword> <code-block-identifier>member</code-block-identifier><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
    <code-block-identifier>foo</code-block-identifier><code-block-punctuation>(</code-block-punctuation>const foo&lt;T&gt;<code-block-punctuation>&amp;</code-block-punctuation> <code-block-identifier>f</code-block-identifier><code-block-punctuation>)</code-block-punctuation> <code-block-punctuation>=</code-block-punctuation> <code-block-keyword>default</code-block-keyword><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
    //=== do_sth ===//<soft-break></soft-break>
    <code-block-keyword>void</code-block-keyword> <code-block-identifier>do_sth</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation> <code-block-keyword>const</code-block-keyword><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>void</code-block-keyword> <code-block-identifier>do_sth</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword><code-block-punctuation>)</code-block-punctuation> <code-block-keyword>noexcept</code-block-keyword><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>void</code-block-keyword> <code-block-identifier>do_sth</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>float</code-block-keyword><code-block-punctuation>)</code-block-punctuation> <code-block-punctuation>&amp;&amp;</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>void</code-block-keyword> <code-block-identifier>do_sth</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>float</code-block-keyword><code-block-punctuation>)</code-block-punctuation> <code-block-keyword>const</code-block-keyword> <code-block-keyword>volatile</code-block-keyword> <code-block-punctuation>&amp;&amp;</code-block-punctuation> <code-block-keyword>noexcept</code-block-keyword><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-punctuation>};</code-block-punctuation><soft-break></soft-break>
</code-block>
)*");
    }
    SECTION("friend")
    {
        auto file = build_doc_entities(comments, index, "synopsis__friend.cpp", R"(
class foo
{
public:
    friend struct bar1;
    friend void func1();
    friend void func2() {}

private:
    friend struct bar2;
    friend void func3();
    friend void func4() {}
};
)");

        auto& foo      = get_named_entity(*file, "foo");
        auto  synopsis = generate_synopsis({}, index, foo);
        REQUIRE(
            markup::as_xml(*synopsis)
            == R"(<code-block language="cpp"><code-block-keyword>class</code-block-keyword> <code-block-identifier>foo</code-block-identifier><soft-break></soft-break>
<code-block-punctuation>{</code-block-punctuation><soft-break></soft-break>
<code-block-keyword>public</code-block-keyword><code-block-punctuation>:</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>friend</code-block-keyword> <code-block-keyword>struct</code-block-keyword> <code-block-identifier>bar1</code-block-identifier><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
    <code-block-keyword>friend</code-block-keyword> <code-block-keyword>void</code-block-keyword> <code-block-identifier>func1</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
    <code-block-keyword>friend</code-block-keyword> <code-block-keyword>void</code-block-keyword> <code-block-identifier>func2</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>private</code-block-keyword><code-block-punctuation>:</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>friend</code-block-keyword> <code-block-keyword>void</code-block-keyword> <code-block-identifier>func4</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-punctuation>};</code-block-punctuation><soft-break></soft-break>
</code-block>
)");
    }
    SECTION("explicit exclude")
    {
        auto file = build_doc_entities(comments, index, "synopsis__explicit_exclude.cpp", R"(
#include <cstddef>
using namespace std;

/// \exclude
void excluded1();

/// \exclude
namespace excluded2
{
    void excluded3();
}

/// \exclude target
using exclude_target1 = int;
/// \exclude target
namespace exclude_target2 = std;

/// \exclude return
int exclude_return1();
/// \exclude return
auto exclude_return2() -> const int&;
)");

        auto synopsis = generate_synopsis({}, index, *file);
        REQUIRE(
            markup::as_xml(*synopsis)
            == R"(<code-block language="cpp"><code-block-keyword>using</code-block-keyword> <code-block-keyword>namespace</code-block-keyword> <code-block-identifier>std</code-block-identifier><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>using</code-block-keyword> <code-block-identifier>exclude_target1</code-block-identifier> <code-block-punctuation>=</code-block-punctuation> <code-block-identifier>&apos;hidden&apos;</code-block-identifier><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>namespace</code-block-keyword> <code-block-identifier>exclude_target2</code-block-identifier> <code-block-punctuation>=</code-block-punctuation> <code-block-identifier>&apos;hidden&apos;</code-block-identifier><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-identifier>&apos;hidden&apos;</code-block-identifier> <code-block-identifier>exclude_return1</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-identifier>&apos;hidden&apos;</code-block-identifier> <code-block-identifier>exclude_return2</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
)");
    }
    SECTION("private excluded")
    {
        auto file = build_doc_entities(comments, index, "synopsis__private_excluded.cpp", R"(
struct base1 {};
struct base2 {};
struct base3 {};

class foo : public base1, private base2, protected base3
{
public:
   void a();
protected:
   void b();
private:
   void c();
   virtual void d();
};
)");

        auto& foo      = get_named_entity(*file, "foo");
        auto  synopsis = generate_synopsis({}, index, foo);
        REQUIRE(
            markup::as_xml(*synopsis)
            == R"(<code-block language="cpp"><code-block-keyword>class</code-block-keyword> <code-block-identifier>foo</code-block-identifier><soft-break></soft-break>
<code-block-punctuation>:</code-block-punctuation> <code-block-keyword>public</code-block-keyword> <code-block-identifier>base1</code-block-identifier><code-block-punctuation>,</code-block-punctuation> <code-block-keyword>protected</code-block-keyword> <code-block-identifier>base3</code-block-identifier><soft-break></soft-break>
<code-block-punctuation>{</code-block-punctuation><soft-break></soft-break>
<code-block-keyword>public</code-block-keyword><code-block-punctuation>:</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>protected</code-block-keyword><code-block-punctuation>:</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>void</code-block-keyword> <code-block-identifier>b</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>private</code-block-keyword><code-block-punctuation>:</code-block-punctuation><soft-break></soft-break>
    <code-block-keyword>virtual</code-block-keyword> <code-block-keyword>void</code-block-keyword> <code-block-identifier>d</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-punctuation>};</code-block-punctuation><soft-break></soft-break>
</code-block>
)");
    }
    SECTION("excluding parameters")
    {
        auto file = build_doc_entities(comments, index, "synopsis__excluding_parameters.cpp", R"(
/// \param a
/// \exclude
void a(int a, int b, int c);
/// \param b
/// \exclude
void b(int a, int b, int c);
/// \param a
/// \exclude
/// \param c
/// \exclude
void c(int a, int b, int c);

/// \param 1
/// \exclude
template <typename T, typename = void>
void d();
/// \param T
/// \exclude
template <typename T>
void e();
)");

        auto synopsis = generate_synopsis({}, index, *file);
    }
    SECTION("mentioning excluded")
    {
        auto file = build_doc_entities(comments, index, "synopsis__mentioning_excluded.cpp", R"(
/// \exclude
namespace foo
{
    template <typename T>
    struct excluded {};
}

using foo::excluded;

// return
foo::excluded<int> a();
const foo::excluded<int>& b();

// parameter
void c(foo::excluded<int> e);
void d(foo::excluded<int>* e);

// target
using e = excluded<int>;
)");

        auto synopsis = generate_synopsis({}, index, *file);
        REQUIRE(
            markup::as_xml(*synopsis)
            == R"(<code-block language="cpp"><code-block-identifier>&apos;hidden&apos;</code-block-identifier> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-identifier>&apos;hidden&apos;</code-block-identifier> <code-block-keyword>const</code-block-keyword><code-block-punctuation>&amp;</code-block-punctuation> <code-block-identifier>b</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>c</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-identifier>&apos;hidden&apos;</code-block-identifier> <code-block-identifier>e</code-block-identifier><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>d</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-identifier>&apos;hidden&apos;</code-block-identifier><code-block-punctuation>*</code-block-punctuation> <code-block-identifier>e</code-block-identifier><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>using</code-block-keyword> <code-block-identifier>e</code-block-identifier> <code-block-punctuation>=</code-block-punctuation> <code-block-identifier>&apos;hidden&apos;</code-block-identifier><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
)");
    }
    SECTION("synopsis override")
    {
        auto file = build_doc_entities(comments, index, "synopsis__synopsis_override.cpp", R"(
/// \synopsis synopsis is overriden
void foo();
)");

        auto synopsis = generate_synopsis({}, index, *file);
        REQUIRE(markup::as_xml(*synopsis)
                == R"(<code-block language="cpp">synopsis is overriden<soft-break></soft-break>
</code-block>
)");
    }
    SECTION("output section")
    {
        auto file = build_doc_entities(comments, index, "synopsis__output_section.cpp", R"(
/// \file
/// \output_section Will not be shown.

/// \output_section First
void a();

void b();

/// \output_section Second
void c();
)");

        auto synopsis = generate_synopsis({}, index, *file);
        REQUIRE(markup::as_xml(*synopsis)
                == R"(<code-block language="cpp">//=== First ===//<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>b</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
//=== Second ===//<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>c</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
)");
    }
    SECTION("groups")
    {
        auto file = build_doc_entities(comments, index, "synopsis__groups.cpp", R"(
void foo();

/// \group a
void a();

/// \group a
void a(int);

void bar();

/// \group -b
void b();

/// \group c Heading
void c();

/// \group b
void b(int);

/// \group a
void a(float);

void baz();
)");

        auto synopsis = generate_synopsis({}, index, *file);
        REQUIRE(
            markup::as_xml(*synopsis)
            == R"(<code-block language="cpp"><code-block-keyword>void</code-block-keyword> <code-block-identifier>foo</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
//=== a ===//<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>float</code-block-keyword><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>bar</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>b</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>b</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
//=== Heading ===//<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>c</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>baz</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
)");

        auto& group          = get_named_entity(*file, "a");
        auto  group_synopsis = generate_synopsis({}, index, group);
        REQUIRE(
            markup::as_xml(*group_synopsis)
            == R"(<code-block language="cpp">(1) <code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
(2) <code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
(3) <code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>float</code-block-keyword><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
)");

        synopsis_config config;
        config.set_flag(synopsis_config::show_group_output_section, false);
        auto synopsis_no_output_section = generate_synopsis(config, index, *file);
        REQUIRE(
            markup::as_xml(*synopsis_no_output_section)
            == R"(<code-block language="cpp"><code-block-keyword>void</code-block-keyword> <code-block-identifier>foo</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>a</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>float</code-block-keyword><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>bar</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>b</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>b</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-keyword>int</code-block-keyword><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>c</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
<soft-break></soft-break>
<code-block-keyword>void</code-block-keyword> <code-block-identifier>baz</code-block-identifier><code-block-punctuation>(</code-block-punctuation><code-block-punctuation>)</code-block-punctuation><code-block-punctuation>;</code-block-punctuation><soft-break></soft-break>
</code-block>
)");
    }
}
