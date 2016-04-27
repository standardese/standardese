// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_variable.hpp>

#include <catch.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("cpp_variable", "[cpp]")
{
    parser p;

    auto code = R"(
        extern char a;
        static char *b = &a;
        const int c = 4;
        const float * const d = nullptr;

        struct foo {};
        foo e;

        unsigned int func(int x);

        constexpr unsigned int f = 4;
        thread_local auto g = func(4);
    )";

    auto tu = parse(p, "cpp_variable", code);

    auto f = tu.parse();
    auto count = 0u;
    p.for_each_in_namespace([&](const cpp_entity &e)
    {
        if (auto var = dynamic_cast<const cpp_variable*>(&e))
        {
            REQUIRE(var->get_unique_name() == var->get_name());
            auto& type = var->get_type();

            if (var->get_name() == "a")
            {
                ++count;
                REQUIRE(type.get_name() == "char");
                REQUIRE(type.get_full_name() == "char");
                REQUIRE(var->get_initializer() == "");
                REQUIRE(var->get_linkage() == cpp_external_linkage);
                REQUIRE(!var->is_thread_local());
            }
            else if (var->get_name() == "b")
            {
                ++count;
                REQUIRE(type.get_name() == "char*");
                REQUIRE(type.get_full_name() == "char *");
                REQUIRE(var->get_initializer() == "&a");
                REQUIRE(var->get_linkage() == cpp_internal_linkage);
                REQUIRE(!var->is_thread_local());
            }
            else if (var->get_name() == "c")
            {
                ++count;
                REQUIRE(type.get_name() == "const int");
                REQUIRE(type.get_full_name() == "const int");
                REQUIRE(var->get_initializer() == "4");
                REQUIRE(var->get_linkage() == cpp_internal_linkage);
                REQUIRE(!var->is_thread_local());
            }
            else if (var->get_name() == "d")
            {
                ++count;
                REQUIRE(type.get_name() == "const float* const");
                REQUIRE(type.get_full_name() == "const float *const");
                REQUIRE(var->get_initializer() == "nullptr");
                REQUIRE(var->get_linkage() == cpp_internal_linkage);
                REQUIRE(!var->is_thread_local());
            }
            else if (var->get_name() == "e")
            {
                ++count;
                REQUIRE(type.get_name() == "foo");
                REQUIRE(type.get_full_name() == "foo");
                REQUIRE(var->get_initializer() == "");
                REQUIRE(var->get_linkage() == cpp_no_linkage);
                REQUIRE(!var->is_thread_local());
            }
            else if (var->get_name() == "f")
            {
                ++count;
                REQUIRE(type.get_name() == "constexpr unsigned int");
                REQUIRE(type.get_full_name() == "const unsigned int");
                REQUIRE(var->get_initializer() == "4");
                REQUIRE(var->get_linkage() == cpp_internal_linkage);
                REQUIRE(!var->is_thread_local());
            }
            else if (var->get_name() == "g")
            {
                ++count;
                REQUIRE(type.get_name() == "auto");
                REQUIRE(type.get_full_name() == "auto");
                REQUIRE(var->get_initializer() == "func(4)");
                REQUIRE(var->get_linkage() == cpp_no_linkage);
                REQUIRE(var->is_thread_local());
            }
            else
                REQUIRE(false);
        }
    });
    REQUIRE(count == 7);
}
