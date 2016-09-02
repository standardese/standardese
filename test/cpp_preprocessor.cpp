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

        #define A

        #define B foo

        #define C(x) x

        #define D(x, ...) __VA_ARGS__

        #define E(...) __VA_ARGS__

        #ifdef __cplusplus
            #define F 0
        #else
            #define F 1
        #endif

        struct ignore_me; // macro cannot be put last
    )";

    auto tu = parse(p, "cpp_preprocessor", code);

    auto count = 0u;
    for (auto& e : tu.get_file())
    {
        if (e.get_name() == "inclusion directive")
        {
            ++count;
            auto& inc = dynamic_cast<const cpp_inclusion_directive&>(e);
            REQUIRE(inc.get_file_name() == "iostream");
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
        else if (e.get_name() == "F")
        {
            ++count;
            auto& macro = dynamic_cast<const cpp_macro_definition&>(e);
            REQUIRE(macro.get_argument_string() == "");
            REQUIRE(macro.get_replacement() == "0");
        }
        else
            REQUIRE(false);
    }
    REQUIRE(count == 7u);
}
