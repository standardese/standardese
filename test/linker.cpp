// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/linker.hpp>

#include <catch.hpp>

#include <standardese/markup/document.hpp>

#include "test_parser.hpp"

using namespace standardese;

bool equal_destination(
    type_safe::variant<type_safe::nullvar_t, markup::block_reference, markup::url> dest,
    const markup::document_entity& doc, const markup::block_id& id)
{
    if (auto ref = dest.optional_value(type_safe::variant_type<markup::block_reference>{}))
        return ref.value().document().value().name() == doc.output_name().name()
               && ref.value().id() == id;
    else
        return false;
}

bool equal_destination(
    type_safe::variant<type_safe::nullvar_t, markup::block_reference, markup::url> dest,
    const char*                                                                    given_url)
{
    if (auto url = dest.optional_value(type_safe::variant_type<markup::url>{}))
        return url.value().as_str() == given_url;
    else
        return false;
}

TEST_CASE("linker")
{
    static auto document_a = markup::main_document::builder("a", "a").finish();
    static auto document_b = markup::main_document::builder("a", "a").finish();

    linker l;

    SECTION("basic")
    {
        REQUIRE(l.register_documentation("foo", *document_a, markup::block_id("foo"), false));
        REQUIRE(l.register_documentation("bar", *document_a, markup::block_id("bar"), false));
        REQUIRE(l.register_documentation("baz", *document_b, markup::block_id("baz"), false));
        REQUIRE(!l.register_documentation("foo", *document_b, markup::block_id("foo"), false));

        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "foo"), *document_a,
                                  markup::block_id("foo")));
        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "bar"), *document_a,
                                  markup::block_id("bar")));
        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "baz"), *document_a,
                                  markup::block_id("baz")));

        // ignore whitespace
        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "  f  o  o  "), *document_a,
                                  markup::block_id("foo")));
        // ignore trailing '()'
        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "foo()"), *document_a,
                                  markup::block_id("foo")));
    }
    SECTION("forcing")
    {
        REQUIRE(l.register_documentation("foo", *document_a, markup::block_id("foo"), false));
        REQUIRE(l.register_documentation("foo", *document_b, markup::block_id("foo"), true));

        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "foo"), *document_b,
                                  markup::block_id("foo")));
    }
    SECTION("short and long link names")
    {
        REQUIRE(l.register_documentation("foo()", *document_a, markup::block_id("foo"), false));
        REQUIRE(l.register_documentation("foo<T>::bar(int).a", *document_a, markup::block_id("bar"),
                                         false));
        REQUIRE(l.register_documentation("foo<T>::baz(int)", *document_b, markup::block_id("baz"),
                                         false));
        REQUIRE(l.register_documentation("foo<T>::baz(short)", *document_b,
                                         markup::block_id("baz2"), false));

        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "foo()"), *document_a,
                                  markup::block_id("foo")));
        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "foo"), *document_a,
                                  markup::block_id("foo")));

        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "foo<T>::bar(int).a"),
                                  *document_a, markup::block_id("bar")));
        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "foo::bar().a"), *document_a,
                                  markup::block_id("bar")));
        REQUIRE(!l.lookup_documentation(nullptr, "foo::bar.a"));

        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "foo<T>::baz(int)"), *document_a,
                                  markup::block_id("baz")));
        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "foo<T>::baz(short)"),
                                  *document_a, markup::block_id("baz2")));
        REQUIRE(!l.lookup_documentation(nullptr, "foo::baz"));
    }
    SECTION("relative name lookup")
    {
        auto file = parse_file({}, "linker__relative_name_lookup.cpp", R"(
void func();

namespace ns
{
    void func();

    template <typename T>
    struct type
    {
         void mfunc(int param);

         void context1();
    };

    void context2();
}

void context3();
)");
        // register non-context entities
        REQUIRE(l.register_documentation("func()", *document_a, markup::block_id("func"), false));
        REQUIRE(l.register_documentation("ns", *document_a, markup::block_id("ns"), false));
        REQUIRE(l.register_documentation("ns::func()", *document_a, markup::block_id("ns::func"),
                                         false));
        REQUIRE(l.register_documentation("ns::type<T>", *document_a, markup::block_id("ns::type"),
                                         false));
        REQUIRE(l.register_documentation("ns::type<T>::mfunc(int)", *document_a,
                                         markup::block_id("ns::type::mfunc"), false));
        REQUIRE(l.register_documentation("ns::type<T>::mfunc(int).param", *document_a,
                                         markup::block_id("ns::type::mfunc.param"), false));

        // lookup from context1
        auto& context1 = get_named_entity(*file, "context1");
        REQUIRE(equal_destination(l.lookup_documentation(type_safe::ref(context1), "*mfunc"),
                                  *document_a, markup::block_id("ns::type::mfunc")));
        REQUIRE(equal_destination(l.lookup_documentation(type_safe::ref(context1), "*func"),
                                  *document_a, markup::block_id("ns::func")));

        // lookup from context2
        auto& context2 = get_named_entity(*file, "context2");
        REQUIRE(equal_destination(l.lookup_documentation(type_safe::ref(context2), "*func"),
                                  *document_a, markup::block_id("ns::func")));

        // lookup from context3
        auto& context3 = get_named_entity(*file, "context3");
        REQUIRE(equal_destination(l.lookup_documentation(type_safe::ref(context3), "*func"),
                                  *document_a, markup::block_id("func")));
    }
    SECTION("external doc")
    {
        l.register_external("std", "std/$$/");
        l.register_external("dts", "dts/$$/");

        REQUIRE(l.register_documentation("foo", *document_a, markup::block_id("foo"), false));
        REQUIRE(
            l.register_documentation("std::foo", *document_a, markup::block_id("std::foo"), false));
        REQUIRE(
            l.register_documentation("std_foo", *document_a, markup::block_id("std_foo"), false));

        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "std::foo"), "std/std::foo/"));
        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "std::bar"), "std/std::bar/"));
        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "dts::foo"), "dts/dts::foo/"));

        REQUIRE(equal_destination(l.lookup_documentation(nullptr, "std_foo"), *document_a,
                                  markup::block_id("std_foo")));

        REQUIRE(!l.lookup_documentation(nullptr, "std_bar"));
    }
}
