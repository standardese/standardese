// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_namespace.hpp>

#include <catch.hpp>
#include <ostream>

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
        auto file = tu.parse();

        auto i = 0u;
        for (auto& e : *file)
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
        auto file = tu.parse();

        auto i = 0u;
        for (auto& e : *file)
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

        auto f_a = tu_a.parse();
        auto f_b = tu_b.parse();

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
