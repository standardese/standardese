// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_preprocessor.hpp>

#include <catch.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("cpp_preprocessor", "[cpp]")
{
    parser p;

    auto code = R"(
        #include <iostream>

        #include "cpp_variable"

        #define A

        #define B foo

        #define C(x) x

        #define D(x, ...) __VA_ARGS__

        #define E(...) __VA_ARGS__

        struct ignore_me; // macro cannot be put last
    )";

    auto tu = parse(p, "cpp_preprocessor", code);
    auto f = tu.parse();
    auto count = 0u;
    p.for_each_in_namespace([&](const cpp_entity &e)
    {
        if (e.get_name() == "iostream")
        {
            ++count;
            auto& inc = dynamic_cast<const cpp_inclusion_directive&>(e);
            REQUIRE(inc.get_kind() == cpp_inclusion_directive::system);
        }
        else if (e.get_name() == "cpp_variable")
        {
            ++count;
            auto& inc = dynamic_cast<const cpp_inclusion_directive&>(e);
            REQUIRE(inc.get_kind() == cpp_inclusion_directive::local);
        }
        else if (e.get_name() == "A")
        {
            ++count;
            auto& macro = dynamic_cast<const cpp_macro_definition&>(e);
            REQUIRE(macro.get_argument_string() == "");
            REQUIRE(macro.get_replacement() == "");
        }
        else if (e.get_name() == "B")
        {
            ++count;
            auto& macro = dynamic_cast<const cpp_macro_definition&>(e);
            REQUIRE(macro.get_argument_string() == "");
            REQUIRE(macro.get_replacement() == "foo");
        }
        else if (e.get_name() == "C")
        {
            ++count;
            auto& macro = dynamic_cast<const cpp_macro_definition&>(e);
            REQUIRE(macro.get_argument_string() == "(x)");
            REQUIRE(macro.get_replacement() == "x");
        }
        else if (e.get_name() == "D")
        {
            ++count;
            auto& macro = dynamic_cast<const cpp_macro_definition&>(e);
            REQUIRE(macro.get_argument_string() == "(x, ...)");
            REQUIRE(macro.get_replacement() == "__VA_ARGS__");
        }
        else if (e.get_name() == "E")
        {
            ++count;
            auto& macro = dynamic_cast<const cpp_macro_definition&>(e);
            REQUIRE(macro.get_argument_string() == "(...)");
            REQUIRE(macro.get_replacement() == "__VA_ARGS__");
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 7u);
}
