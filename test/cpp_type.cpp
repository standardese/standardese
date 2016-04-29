// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_enum.hpp>
#include <standardese/cpp_type.hpp>

#include <catch.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("cpp_type_alias", "[cpp]")
{
    parser p;

    auto code = R"(
        using type_1 = int;
        using type_2 = char[];

        typedef int type_3;

        struct foo {};

        using type_4 = const foo;

        namespace ns
        {
            struct foo {};

            using type_5 = foo;
        }

        using type_6 = ns::foo;

        typedef void(*type_7)(int, char);
    )";

    auto tu = parse(p, "cpp_type_alias", code);

    auto f = tu.parse();
    auto count = 0u;
    p.for_each_type([&](const cpp_type &e)
    {
        auto& t = dynamic_cast<const cpp_type_alias&>(e);
        if (t.get_name() == "type_1")
        {
            ++count;
            REQUIRE(t.get_unique_name() == "type_1");
            auto& target = t.get_target();
            REQUIRE(target.get_name() == "int");
            REQUIRE(target.get_full_name() == "int");
        }
        else if (t.get_name() == "type_2")
        {
            ++count;
            REQUIRE(t.get_unique_name() == "type_2");
            auto& target = t.get_target();
            REQUIRE(target.get_name() == "char[]");
            // note: whitespace because libclang inserts space there
            REQUIRE(target.get_full_name() == "char []");
        }
        else if (t.get_name() == "type_3")
        {
            ++count;
            REQUIRE(t.get_unique_name() == "type_3");
            auto& target = t.get_target();
            REQUIRE(target.get_name() == "int");
            REQUIRE(target.get_full_name() == "int");
        }
        else if (t.get_name() == "type_4")
        {
            ++count;
            REQUIRE(t.get_unique_name() == "type_4");
            auto& target = t.get_target();
            REQUIRE(target.get_name() == "const foo");
            REQUIRE(target.get_full_name() == "const foo");
        }
        else if (t.get_name() == "type_5")
        {
            ++count;
            REQUIRE(t.get_unique_name() == "ns::type_5");
            auto& target = t.get_target();
            REQUIRE(target.get_name() == "foo");
            REQUIRE(target.get_full_name() == "ns::foo");
        }
        else if (t.get_name() == "type_6")
        {
            ++count;
            REQUIRE(t.get_unique_name() == "type_6");
            auto& target = t.get_target();
            REQUIRE(target.get_name() == "ns::foo");
            REQUIRE(target.get_full_name() == "ns::foo");
        }
        else if (t.get_name() == "type_7")
        {
            ++count;
            REQUIRE(t.get_unique_name() == "type_7");
            auto& target = t.get_target();
            REQUIRE(target.get_name() == "void(*)(int, char)");
            REQUIRE(target.get_full_name() == "void (*)(int, char)");
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 7u);
}

TEST_CASE("cpp_enum", "[cpp]")
{
    parser p;

    auto code = R"(
        enum a
        {
            a_1,
            a_2,
            a_3 = 5
        };

        enum class b : int
        {
            b_1,
            b_2,
            b_3 = -1
        };
    )";

    auto tu = parse(p, "cpp_enum", code);

    auto f = tu.parse();
    auto count = 0u;
    p.for_each_type([&](const cpp_type &e)
    {
        auto &t = dynamic_cast<const cpp_enum&>(e);
        REQUIRE(t.get_name() == t.get_unique_name());

        if (t.get_name() == "a")
        {
            REQUIRE(!t.is_scoped());
            auto& underlying = t.get_underlying_type();
            REQUIRE(underlying.get_name() == "");
            REQUIRE(underlying.get_full_name() == "unsigned int");

            auto i = 1u;
            for (auto& val : t)
            {
                auto& eval = dynamic_cast<const cpp_unsigned_enum_value&>(val);
                REQUIRE(eval.get_name() == "a_" + std::to_string(i));
                REQUIRE(eval.get_unique_name() == "a_" + std::to_string(i));

                if (i == 3u)
                {
                    REQUIRE(eval.get_value() == 5);
                    REQUIRE(eval.is_explicitly_given());
                }
                else
                {
                    REQUIRE(eval.get_value() == i - 1);
                    REQUIRE(!eval.is_explicitly_given());
                }
                ++i;
            }
            REQUIRE(i == 4u);

            ++count;
        }
        else if (t.get_name() == "b")
        {
            REQUIRE(t.is_scoped());
            auto& underlying = t.get_underlying_type();
            REQUIRE(underlying.get_name() == "int");
            REQUIRE(underlying.get_full_name() == "int");

            auto i = 1u;
            for (auto& val : t)
            {
                auto& eval = dynamic_cast<const cpp_signed_enum_value&>(val);
                REQUIRE(eval.get_name() == "b_" + std::to_string(i));
                REQUIRE(eval.get_unique_name() == "b::b_" + std::to_string(i));

                if (i == 3u)
                {
                    REQUIRE(eval.get_value() == -1);
                    REQUIRE(eval.is_explicitly_given());
                }
                else
                {
                    REQUIRE(eval.get_value() == i - 1);
                    REQUIRE(!eval.is_explicitly_given());
                }
                ++i;
            }
            REQUIRE(i == 4u);

            ++count;
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 2u);
}
