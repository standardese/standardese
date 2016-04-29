// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_function.hpp>

#include <catch.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("cpp_function", "[cpp]")
{
    parser p;

    auto code = R"(
        void a(int x, const char *ptr = nullptr);

        int b(int c, ...)
        {
            return 0;
        }

        int *c(int a = b(0));

        char &d() noexcept;

        const int e() noexcept(false);

        int (*f(int a))(volatile char&&);

        constexpr auto g() -> const char&&;

        auto h() noexcept(noexcept(e()))
        {
            return 0;
        }

        decltype(auto) i();
    )";

    auto tu = parse(p, "cpp_function", code);
    auto f = tu.parse();

    // no need to check the parameters, same code as for variables
    auto count = 0u;
    p.for_each_in_namespace([&](const cpp_entity &e)
    {
        auto& func = dynamic_cast<const cpp_function&>(e);
        REQUIRE(func.get_name() == func.get_unique_name());
        REQUIRE(func.get_definition() == cpp_function_definition_normal);

        INFO(func.get_return_type().get_full_name());
        if (func.get_name() == "a")
        {
            ++count;
            REQUIRE(func.get_return_type().get_name() == "void");
            REQUIRE(!func.is_constexpr());
            REQUIRE(!func.is_variadic());
            REQUIRE(func.get_noexcept() == "false");
        }
        else if (func.get_name() == "b")
        {
            ++count;
            REQUIRE(func.get_return_type().get_name() == "int");
            REQUIRE(!func.is_constexpr());
            REQUIRE(func.is_variadic());
            REQUIRE(func.get_noexcept() == "false");
        }
        else if (func.get_name() == "c")
        {
            ++count;
            REQUIRE(func.get_return_type().get_name() == "int *");
            REQUIRE(!func.is_constexpr());
            REQUIRE(!func.is_variadic());
            REQUIRE(func.get_noexcept() == "false");
        }
        else if (func.get_name() == "d")
        {
            ++count;
            REQUIRE(func.get_return_type().get_name() == "char &");
            REQUIRE(!func.is_constexpr());
            REQUIRE(!func.is_variadic());
            REQUIRE(func.get_noexcept() == "true");
        }
        else if (func.get_name() == "e")
        {
            ++count;
            REQUIRE(func.get_return_type().get_name() == "const int");
            REQUIRE(!func.is_constexpr());
            REQUIRE(!func.is_variadic());
            REQUIRE(func.get_noexcept() == "false");
        }
        else if (func.get_name() == "f")
        {
            ++count;
            REQUIRE(func.get_return_type().get_name() == "int(*)(volatile char &&)");
            REQUIRE(!func.is_constexpr());
            REQUIRE(!func.is_variadic());
            REQUIRE(func.get_noexcept() == "false");
        }
        else if (func.get_name() == "g")
        {
            ++count;
            REQUIRE(func.get_return_type().get_name() == "const char &&");
            REQUIRE(func.is_constexpr());
            REQUIRE(!func.is_variadic());
            REQUIRE(func.get_noexcept() == "false");
        }
        else if (func.get_name() == "h")
        {
            ++count;
            REQUIRE(func.get_return_type().get_name() == "auto");
            REQUIRE(!func.is_constexpr());
            REQUIRE(!func.is_variadic());
            REQUIRE(func.get_noexcept() == "noexcept(e())");
        }
        else if (func.get_name() == "i")
        {
            ++count;
            REQUIRE(func.get_return_type().get_name() == "decltype(auto)");
            REQUIRE(!func.is_constexpr());
            REQUIRE(!func.is_variadic());
            REQUIRE(func.get_noexcept() == "false");
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 9u);
}