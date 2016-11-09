// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/preprocessor.hpp>

#include <catch.hpp>

#include <standardese/doc_entity.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("cpp_macro_definition", "[cpp]")
{
    parser p(test_logger);

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

        /// test
        struct test {};
    )";

    auto tu = parse(p, "cpp_preprocessor", code);

    auto count = 0u;
    for (auto& e : tu.get_file())
    {
        if (e.get_name() == "A")
        {
            ++count;
            auto& macro = dynamic_cast<const cpp_macro_definition&>(e);
            REQUIRE(macro.get_parameter_string() == "");
            REQUIRE(macro.get_replacement() == "");
        }
        else if (e.get_name() == "B")
        {
            ++count;
            auto& macro = dynamic_cast<const cpp_macro_definition&>(e);
            REQUIRE(macro.get_parameter_string() == "");
            REQUIRE(macro.get_replacement() == "foo");
        }
        else if (e.get_name() == "C")
        {
            ++count;
            auto& macro = dynamic_cast<const cpp_macro_definition&>(e);
            REQUIRE(macro.get_parameter_string() == "(x)");
            REQUIRE(macro.get_replacement() == "x");
        }
        else if (e.get_name() == "D")
        {
            ++count;
            auto& macro = dynamic_cast<const cpp_macro_definition&>(e);
            REQUIRE(macro.get_parameter_string() == "(x, ...)");
            REQUIRE(macro.get_replacement() == "__VA_ARGS__");
        }
        else if (e.get_name() == "E")
        {
            ++count;
            auto& macro = dynamic_cast<const cpp_macro_definition&>(e);
            REQUIRE(macro.get_parameter_string() == "(...)");
            REQUIRE(macro.get_replacement() == "__VA_ARGS__");
        }
        else if (e.get_name() == "F")
        {
            ++count;
            auto& macro = dynamic_cast<const cpp_macro_definition&>(e);
            REQUIRE(macro.get_parameter_string() == "");
            REQUIRE(macro.get_replacement() == "0");
        }
        else if (e.get_name() == "test")
            REQUIRE(p.get_comment_registry().lookup_comment(p.get_entity_registry(), e) != nullptr);
        else
            REQUIRE(false);
    }
    REQUIRE(count == 6u);
}
