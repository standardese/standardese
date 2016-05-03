// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_namespace.hpp>

#include <catch.hpp>
#include <standardese/cpp_class.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("cpp_namespace", "[cpp]")
{
    parser p;

    SECTION("basic parsing")
    {
        auto code = R"(
            namespace ns_0 {}
            inline namespace ns_1 {}
        )";
        auto tu = parse(p, "cpp_namespace__basic_parsing", code);
        auto& file = tu.build_ast();

        auto i = 0u;
        for (auto& e : file)
        {
            auto& ns = dynamic_cast<const cpp_namespace&>(e);

            auto name = "ns_" + std::to_string(i);
            REQUIRE(ns.get_name() == name);
            REQUIRE(ns.get_unique_name() == name);

            REQUIRE(ns.is_inline() == bool(i));

            REQUIRE(ns.empty());
            ++i;
        }
        REQUIRE(i == 2u);
    }

    SECTION("nested parsing")
    {
        auto code = R"(
            namespace ns_0
            {
                namespace ns_0_0 {}
                namespace ns_0_1
                {
                    namespace ns_0_1_0 {}
                }
            }

            namespace ns_1 {}
        )";

        auto tu = parse(p, "cpp_namespace__nested_parsing", code);
        auto& file = tu.build_ast();

        auto i = 0u;
        for (auto& e : file)
        {
            auto& ns = dynamic_cast<const cpp_namespace&>(e);

            auto name = "ns_" + std::to_string(i);
            REQUIRE(ns.get_name() == name);
            REQUIRE(ns.get_unique_name() == name);

            if (i == 0u)
            {
                auto j = 0u;
                for (auto& e : ns)
                {
                    auto& ns = dynamic_cast<const cpp_namespace&>(e);

                    auto name = "ns_0_" + std::to_string(j);
                    REQUIRE(ns.get_name() == name);
                    REQUIRE(ns.get_unique_name() == "ns_0::" + name);

                    if (j == 0u)
                        REQUIRE(ns.empty());
                    else if (j == 1u)
                        REQUIRE(std::next(ns.begin()) == ns.end());

                    ++j;
                }

                REQUIRE(j == 2u);
            }
            else if (i == 1u)
                REQUIRE(ns.empty());

            ++i;
        }
        REQUIRE(i == 2u);
    }
    SECTION("multiple tu")
    {
        auto code_a = R"(
            namespace outer
            {
                namespace inner {}
            }
        )";
        auto code_b = R"(
            namespace outer {}
            namespace inner {}
        )";

        auto tu_a = parse(p, "cpp_namespace__multiple_tu__a", code_a);
        auto tu_b = parse(p, "cpp_namespace__multiple_tu__b", code_b);

        tu_a.build_ast();
        tu_b.build_ast();

        std::set<cpp_name> names;
        names.insert("outer");
        names.insert("inner");
        names.insert("outer::inner");
        p.for_each_namespace([&](const cpp_name &n)
                             {
                                 auto iter = names.find(n);
                                 REQUIRE(iter != names.end());
                                 names.erase(iter);
                             });
        REQUIRE(names.empty());
    }
}

TEST_CASE("cpp_namespace_alias", "[cpp]")
{
    parser p;

    auto code = R"(
        namespace foo {}
        namespace outer
        {
            namespace inner2
            {
                namespace inner {}
            }

            namespace inner
            {
                namespace e = outer;
                namespace f = inner2;

                namespace g = inner2::inner;
            }

            namespace a = inner;
            namespace b = foo;
        }

        namespace c = outer::inner;
        namespace d = ::foo;

        using namespace outer;

        namespace h = inner2::inner;
        namespace i = a::e;
    )";

    auto tu = parse(p, "cpp_namespace_alias", code);
    tu.build_ast();

    auto count = 0u;
    p.for_each_in_namespace([&](const cpp_entity &e)
    {
        if (auto ptr = dynamic_cast<const cpp_namespace_alias*>(&e))
        {
            if (ptr->get_name() == "a")
            {
                ++count;
                REQUIRE(ptr->get_unique_name() == "outer::a");
                REQUIRE(ptr->get_target() == "inner");
                REQUIRE(ptr->get_full_target() == "outer::inner");
            }
            else if (ptr->get_name() == "b")
            {
                ++count;
                REQUIRE(ptr->get_unique_name() == "outer::b");
                REQUIRE(ptr->get_target() == "foo");
                REQUIRE(ptr->get_full_target() == "foo");
            }
            else if (ptr->get_name() == "c")
            {
                ++count;
                REQUIRE(ptr->get_unique_name() == "c");
                REQUIRE(ptr->get_target() == "outer::inner");
                REQUIRE(ptr->get_full_target() == "outer::inner");
            }
            else if (ptr->get_name() == "d")
            {
                ++count;
                REQUIRE(ptr->get_unique_name() == "d");
                REQUIRE(ptr->get_target() == "foo");
                REQUIRE(ptr->get_full_target() == "foo");
            }
            else if (ptr->get_name() == "e")
            {
                ++count;
                REQUIRE(ptr->get_unique_name() == "outer::inner::e");
                REQUIRE(ptr->get_target() == "outer");
                REQUIRE(ptr->get_full_target() == "outer");
            }
            else if (ptr->get_name() == "f")
            {
                ++count;
                REQUIRE(ptr->get_unique_name() == "outer::inner::f");
                REQUIRE(ptr->get_target() == "inner2");
                REQUIRE(ptr->get_full_target() == "outer::inner2");
            }
            else if (ptr->get_name() == "g")
            {
                ++count;
                REQUIRE(ptr->get_unique_name() == "outer::inner::g");
                REQUIRE(ptr->get_target() == "inner2::inner");
                REQUIRE(ptr->get_full_target() == "outer::inner2::inner");
            }
            else if (ptr->get_name() == "h")
            {
                ++count;
                REQUIRE(ptr->get_unique_name() == "h");
                REQUIRE(ptr->get_target() == "inner2::inner");
                REQUIRE(ptr->get_full_target() == "outer::inner2::inner");
            }
            else if (ptr->get_name() == "i")
            {
                ++count;
                REQUIRE(ptr->get_unique_name() == "i");
                REQUIRE(ptr->get_target() == "a::e");
                REQUIRE(ptr->get_full_target() == "outer::a::e");
            }
            else
                REQUIRE(false);
        }
    });
    REQUIRE(count == 9u);
}

TEST_CASE("cpp_using_directive", "[cpp]")
{
    parser p;

    auto code = R"(
        namespace foo {}
        namespace outer
        {
            namespace inner {}

            /// a
            using namespace inner;
        }

        /// b
        using namespace foo;
        /// c
        using namespace outer::inner;
        /// d
        using namespace outer;
        /// e
        using namespace inner;

        namespace outer2
        {
            namespace inner {}
        }

        namespace bar = outer2::inner;

        /// f
        using namespace bar;
    )";

    auto tu = parse(p, "cpp_namespace_alias", code);
    tu.build_ast();

    auto count = 0u;
    p.for_each_in_namespace([&](const cpp_entity &e)
    {
        if (auto ptr = dynamic_cast<const cpp_using_directive*>(&e))
        {
            if (ptr->get_comment().back() == 'a')
            {
                ++count;
                REQUIRE(ptr->get_name() == "inner");
                REQUIRE(ptr->get_unique_name() == "outer::inner");
            }
            else if (ptr->get_comment().back() == 'b')
            {
                ++count;
                REQUIRE(ptr->get_name() == "foo");
                REQUIRE(ptr->get_unique_name() == "foo");
            }
            else if (ptr->get_comment().back() == 'c')
            {
                ++count;
                REQUIRE(ptr->get_name() == "outer::inner");
                REQUIRE(ptr->get_unique_name() == "outer::inner");
            }
            else if (ptr->get_comment().back() == 'd')
            {
                ++count;
                REQUIRE(ptr->get_name() == "outer");
                REQUIRE(ptr->get_unique_name() == "outer");
            }
            else if (ptr->get_comment().back() == 'e')
            {
                ++count;
                REQUIRE(ptr->get_name() == "inner");
                REQUIRE(ptr->get_unique_name() == "outer::inner");
            }
            else if (ptr->get_comment().back() == 'f')
            {
                ++count;
                REQUIRE(ptr->get_name() == "bar");
                REQUIRE(ptr->get_unique_name() == "bar");
            }
            else
                REQUIRE(false);
        }
    });

    REQUIRE(count == 6u);
}

TEST_CASE("cpp_using_declaration", "[cpp]")
{
    parser p;

    auto code = R"(
        namespace ns
        {
            struct foo;

            namespace inner
            {
                struct bar;
            }

            /// a
            using inner::bar;
        }

        /// b
        using ns::foo;

        /// c
        using ns::inner::bar;

        struct base
        {
            base(int);

            void foo(int);
        };

        struct derived
        : base
        {
            /// d
            using base::base;

            /// e
            using base::foo;
        };
    )";

    auto tu = parse(p, "cpp_using_declaration", code);
    tu.build_ast();

    auto count = 0u;
    p.for_each_in_namespace([&](const cpp_entity &e)
    {
        if (auto ptr = dynamic_cast<const cpp_using_declaration*>(&e))
        {
            if (ptr->get_comment() == "/// a")
            {
                ++count;
                REQUIRE(ptr->get_name() == "inner::bar");
                REQUIRE(ptr->get_unique_name() == "ns::inner::bar");
            }
            else if (ptr->get_comment() == "/// b")
            {
                ++count;
                REQUIRE(ptr->get_name() == "ns::foo");
                REQUIRE(ptr->get_unique_name() == "ns::foo");
            }
            else if (ptr->get_comment() == "/// c")
            {
                ++count;
                REQUIRE(ptr->get_name() == "ns::inner::bar");
                REQUIRE(ptr->get_unique_name() == "ns::inner::bar");
            }
            else
                REQUIRE(false);
        }
        else if (e.get_name() == "derived")
        {
            for (auto& m : dynamic_cast<const cpp_class &>(e))
            {
                if (m.get_name() == "base")
                    continue; // skip base specifier

                auto& ud = dynamic_cast<const cpp_using_declaration&>(m);
                if (ud.get_comment() == "/// d")
                {
                    ++count;
                    REQUIRE(ud.get_name() == "base::base");
                    REQUIRE(ud.get_unique_name() == "base::base");
                }
                else if (ud.get_comment() == "/// e")
                {
                    ++count;
                    REQUIRE(ud.get_name() == "base::foo");
                    REQUIRE(ud.get_unique_name() == "base::foo");
                }
            }
        }
    });
    REQUIRE(count == 5u);
}
