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
        else
            REQUIRE(false);
    });
    REQUIRE(count == 2u);
}
