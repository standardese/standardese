// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_namespace.hpp>

#include <catch.hpp>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/cpp_class.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("cpp_language_linkage", "[cpp]")
{
    parser p;

    auto code = R"(
            /// a
            extern "C"
            {
                struct a {};
                struct b {};
            }

            struct c {};

            /// b
            extern "C++"
            {
                struct d {};
                struct e {};
            }
        )";

    auto tu    = parse(p, "cpp_language_linkage", code);
    auto count = 0;
    for (auto& e : tu.get_file())
    {
        if (e.get_entity_type() == cpp_entity::language_linkage_t)
        {
            ++count;
            auto& lang = dynamic_cast<const cpp_language_linkage&>(e);

            if (detail::parse_comment(lang.get_cursor()) == "/// a")
            {
                REQUIRE(lang.get_name() == "C");

                for (auto& child : lang)
                {
                    auto valid = child.get_name() == "a" || child.get_name() == "b";
                    REQUIRE(valid);
                    REQUIRE(child.get_full_name() == child.get_name());
                }
            }
            else if (detail::parse_comment(lang.get_cursor()) == "/// b")
            {
                REQUIRE(lang.get_name() == "C++");

                for (auto& child : lang)
                {
                    auto valid = child.get_name() == "d" || child.get_name() == "e";
                    REQUIRE(valid);
                    REQUIRE(child.get_full_name() == child.get_name());
                }
            }
            else
                REQUIRE(false);
        }
        else
            REQUIRE(e.get_name() == "c");
    }
    REQUIRE(count == 2u);
}

TEST_CASE("cpp_namespace", "[cpp]")
{
    parser p;

    SECTION("basic parsing")
    {
        auto code = R"(
            namespace ns_0 {}
            inline namespace ns_1 {}
        )";
        auto tu   = parse(p, "cpp_namespace__basic_parsing", code);

        auto i = 0u;
        for (auto& e : tu.get_file())
        {
            auto& ns = dynamic_cast<const cpp_namespace&>(e);

            auto name = "ns_" + std::to_string(i);
            REQUIRE(ns.get_name() == name);
            REQUIRE(ns.get_full_name() == name);

            REQUIRE(ns.is_inline() == !!i);

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

        auto i = 0u;
        for (auto& e : tu.get_file())
        {
            auto& ns = dynamic_cast<const cpp_namespace&>(e);

            auto name = "ns_" + std::to_string(i);
            REQUIRE(ns.get_name() == name);
            REQUIRE(ns.get_full_name() == name);

            if (i == 0u)
            {
                auto j = 0u;
                for (auto& e : ns)
                {
                    auto& ns = dynamic_cast<const cpp_namespace&>(e);

                    auto name = "ns_0_" + std::to_string(j);
                    REQUIRE(ns.get_name() == name);
                    REQUIRE(ns.get_full_name() == "ns_0::" + name);

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

    auto count = 0u;
    for_each(tu.get_file(), [&](const cpp_entity& e) {
        if (auto ptr = dynamic_cast<const cpp_namespace_alias*>(&e))
        {
            if (ptr->get_name() == "a")
            {
                ++count;
                REQUIRE(ptr->get_full_name() == "outer::a");
                REQUIRE(ptr->get_target().get_name() == "inner");
                REQUIRE(ptr->get_target().get_full_name() == "outer::inner");
            }
            else if (ptr->get_name() == "b")
            {
                ++count;
                REQUIRE(ptr->get_full_name() == "outer::b");
                REQUIRE(ptr->get_target().get_name() == "foo");
                REQUIRE(ptr->get_target().get_full_name() == "foo");
            }
            else if (ptr->get_name() == "c")
            {
                ++count;
                REQUIRE(ptr->get_full_name() == "c");
                REQUIRE(ptr->get_target().get_name() == "outer::inner");
                REQUIRE(ptr->get_target().get_full_name() == "outer::inner");
            }
            else if (ptr->get_name() == "d")
            {
                ++count;
                REQUIRE(ptr->get_full_name() == "d");
                REQUIRE(ptr->get_target().get_name() == "::foo");
                REQUIRE(ptr->get_target().get_full_name() == "foo");
            }
            else if (ptr->get_name() == "e")
            {
                ++count;
                REQUIRE(ptr->get_full_name() == "outer::inner::e");
                REQUIRE(ptr->get_target().get_name() == "outer");
                REQUIRE(ptr->get_target().get_full_name() == "outer");
            }
            else if (ptr->get_name() == "f")
            {
                ++count;
                REQUIRE(ptr->get_full_name() == "outer::inner::f");
                REQUIRE(ptr->get_target().get_name() == "inner2");
                REQUIRE(ptr->get_target().get_full_name() == "outer::inner2");
            }
            else if (ptr->get_name() == "g")
            {
                ++count;
                REQUIRE(ptr->get_full_name() == "outer::inner::g");
                REQUIRE(ptr->get_target().get_name() == "inner2::inner");
                REQUIRE(ptr->get_target().get_full_name() == "outer::inner2::inner");
            }
            else if (ptr->get_name() == "h")
            {
                ++count;
                REQUIRE(ptr->get_full_name() == "h");
                REQUIRE(ptr->get_target().get_name() == "inner2::inner");
                REQUIRE(ptr->get_target().get_full_name() == "outer::inner2::inner");
            }
            else if (ptr->get_name() == "i")
            {
                ++count;
                REQUIRE(ptr->get_full_name() == "i");
                REQUIRE(ptr->get_target().get_name() == "a::e");
                REQUIRE(ptr->get_target().get_full_name() == "outer::inner::e");
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

    auto count = 0u;
    for_each(tu.get_file(), [&](const cpp_entity& e) {
        if (auto ptr = dynamic_cast<const cpp_using_directive*>(&e))
        {
            if (*std::prev(detail::parse_comment(ptr->get_cursor()).end()) == 'a')
            {
                ++count;
                REQUIRE(ptr->get_target().get_name() == "inner");
                REQUIRE(ptr->get_target().get_full_name() == "outer::inner");
            }
            else if (*std::prev(detail::parse_comment(ptr->get_cursor()).end()) == 'b')
            {
                ++count;
                REQUIRE(ptr->get_target().get_name() == "foo");
                REQUIRE(ptr->get_target().get_full_name() == "foo");
            }
            else if (*std::prev(detail::parse_comment(ptr->get_cursor()).end()) == 'c')
            {
                ++count;
                REQUIRE(ptr->get_target().get_name() == "outer::inner");
                REQUIRE(ptr->get_target().get_full_name() == "outer::inner");
            }
            else if (*std::prev(detail::parse_comment(ptr->get_cursor()).end()) == 'd')
            {
                ++count;
                REQUIRE(ptr->get_target().get_name() == "outer");
                REQUIRE(ptr->get_target().get_full_name() == "outer");
            }
            else if (*std::prev(detail::parse_comment(ptr->get_cursor()).end()) == 'e')
            {
                ++count;
                REQUIRE(ptr->get_target().get_name() == "inner");
                REQUIRE(ptr->get_target().get_full_name() == "outer::inner");
            }
            else if (*std::prev(detail::parse_comment(ptr->get_cursor()).end()) == 'f')
            {
                ++count;
                REQUIRE(ptr->get_target().get_name() == "bar");
                REQUIRE(ptr->get_target().get_full_name() == "bar");
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

    auto count = 0u;
    for_each(tu.get_file(), [&](const cpp_entity& e) {
        if (auto ptr = dynamic_cast<const cpp_using_declaration*>(&e))
        {
            if (detail::parse_comment(ptr->get_cursor()) == "/// a")
            {
                ++count;
                REQUIRE(ptr->get_target().get_name() == "inner::bar");
                REQUIRE(ptr->get_target().get_full_name() == "ns::inner::bar");
            }
            else if (detail::parse_comment(ptr->get_cursor()) == "/// b")
            {
                ++count;
                REQUIRE(ptr->get_target().get_name() == "ns::foo");
                REQUIRE(ptr->get_target().get_full_name() == "ns::foo");
            }
            else if (detail::parse_comment(ptr->get_cursor()) == "/// c")
            {
                ++count;
                REQUIRE(ptr->get_target().get_name() == "ns::inner::bar");
                REQUIRE(ptr->get_target().get_full_name() == "ns::inner::bar");
            }
            else
                REQUIRE(false);
        }
        else if (e.get_name() == "derived")
        {
            for (auto& m : dynamic_cast<const cpp_class&>(e))
            {
                if (m.get_name() == "base")
                    continue; // skip base specifier

                auto& ud = dynamic_cast<const cpp_using_declaration&>(m);
                if (detail::parse_comment(ud.get_cursor()) == "/// d")
                {
                    ++count;
                    REQUIRE(ud.get_target().get_name() == "base::base");
                    INFO(ud.get_target().get_scope().c_str());
                    REQUIRE(ud.get_target().get_full_name() == "base::base");
                }
                else if (detail::parse_comment(ud.get_cursor()) == "/// e")
                {
                    ++count;
                    REQUIRE(ud.get_target().get_name() == "base::foo");
                    REQUIRE(ud.get_target().get_full_name() == "base::foo");
                }
            }
        }
    });
    REQUIRE(count == 5u);
}
