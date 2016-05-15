// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_variable.hpp>

#include <catch.hpp>
#include <standardese/cpp_class.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("cpp_variable", "[cpp]")
{
    parser p;

    auto code = R"(
        extern char a;
        static char *b = &a;
        constexpr int c = 4;
        const float * const d = nullptr;

        struct foo {};
        foo e;

        unsigned int (*f)(int x) = nullptr;
        thread_local auto g = f(4);

        int h[5];
    )";

    auto tu = parse(p, "cpp_variable", code);
    tu.build_ast();

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
                REQUIRE(type.get_name() == "char *");
                REQUIRE(type.get_full_name() == "char *");
                REQUIRE(var->get_initializer() == "&a");
                REQUIRE(var->get_linkage() == cpp_internal_linkage);
                REQUIRE(!var->is_thread_local());
            }
            else if (var->get_name() == "c")
            {
                ++count;
                REQUIRE(type.get_name() == "constexpr int");
                REQUIRE(type.get_full_name() == "const int");
                REQUIRE(var->get_initializer() == "4");
                REQUIRE(var->get_linkage() == cpp_internal_linkage);
                REQUIRE(!var->is_thread_local());
            }
            else if (var->get_name() == "d")
            {
                ++count;
                REQUIRE(type.get_name() == "const float * const");
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
                REQUIRE(type.get_name() == "unsigned int (*)(int x)");
                REQUIRE(type.get_full_name() == "unsigned int (*)(int)");
                REQUIRE(var->get_initializer() == "nullptr");
                REQUIRE(var->get_linkage() == cpp_no_linkage);
                REQUIRE(!var->is_thread_local());
            }
            else if (var->get_name() == "g")
            {
                ++count;
                REQUIRE(type.get_name() == "auto");
                REQUIRE(type.get_full_name() == "auto");
                REQUIRE(var->get_initializer() == "f(4)");
                REQUIRE(var->get_linkage() == cpp_no_linkage);
                REQUIRE(var->is_thread_local());
            }
            else if (var->get_name() == "h")
            {
                ++count;
                REQUIRE(type.get_name() == "int [5]");
                REQUIRE(type.get_full_name() == "int [5]");
                REQUIRE(var->get_initializer() == "");
                REQUIRE(var->get_linkage() == cpp_no_linkage);
                REQUIRE(!var->is_thread_local());
            }
            else
                REQUIRE(false);
        }
    });
    REQUIRE(count == 8);
}

TEST_CASE("cpp_member_variable and cpp_bitfield", "[cpp]")
{
    parser p;

    auto code = R"(
        struct foo
        {
            int a;

            char b = '4';

            const int c[42];

            mutable float d;

            static int& e;

            int f : 5;
            int g : 3;
            int : 0;
            mutable int h : 1;
        };
    )";

    auto tu = parse(p, "cpp_member_variable", code);
    tu.build_ast();

    auto count = 0u;
    p.for_each_type([&](const cpp_type &t)
    {
        auto& c = dynamic_cast<const cpp_class&>(t);
        for (auto& e : c)
        {
            auto& var = dynamic_cast<const cpp_variable&>(e);
            INFO(var.get_name());
            REQUIRE(var.get_unique_name() == "foo::" + var.get_name());
            REQUIRE(!var.is_thread_local());

            if (var.get_name() != "e")
                REQUIRE(var.get_linkage() == cpp_no_linkage);

            if (var.get_name() == "a")
            {
                ++count;
                REQUIRE(var.get_type().get_name() == "int");
                REQUIRE(var.get_initializer() == "");
                REQUIRE(!dynamic_cast<const cpp_member_variable&>(var).is_mutable());
            }
            else if (var.get_name() == "b")
            {
                ++count;
                REQUIRE(var.get_type().get_name() == "char");
                REQUIRE(var.get_initializer() == "'4'");
                REQUIRE(!dynamic_cast<const cpp_member_variable&>(var).is_mutable());
            }
            else if (var.get_name() == "c")
            {
                ++count;
                REQUIRE(var.get_type().get_name() == "const int [42]");
                REQUIRE(var.get_initializer() == "");
                REQUIRE(!dynamic_cast<const cpp_member_variable&>(var).is_mutable());
            }
            else if (var.get_name() == "d")
            {
                ++count;
                REQUIRE(var.get_type().get_name() == "float");
                REQUIRE(var.get_initializer() == "");
                REQUIRE(dynamic_cast<const cpp_member_variable&>(var).is_mutable());
            }
            else if (var.get_name() == "e")
            {
                ++count;
                REQUIRE(var.get_type().get_name() == "int&");
                REQUIRE(var.get_initializer() == "");
                REQUIRE(var.get_linkage() == cpp_external_linkage);
            }
            else if (var.get_name() == "f")
            {
                ++count;
                REQUIRE(var.get_type().get_name() == "int");
                REQUIRE(var.get_initializer() == "");
                REQUIRE(var.get_linkage() == cpp_no_linkage);
                REQUIRE(!dynamic_cast<const cpp_member_variable&>(var).is_mutable());
                REQUIRE(dynamic_cast<const cpp_bitfield&>(var).no_bits() == 5);
            }
            else if (var.get_name() == "g")
            {
                ++count;
                REQUIRE(var.get_type().get_name() == "int");
                REQUIRE(var.get_initializer() == "");
                REQUIRE(var.get_linkage() == cpp_no_linkage);
                REQUIRE(!dynamic_cast<const cpp_member_variable&>(var).is_mutable());
                REQUIRE(dynamic_cast<const cpp_bitfield&>(var).no_bits() == 3);
            }
            else if (var.get_name() == "")
            {
                ++count;
                REQUIRE(var.get_type().get_name() == "int");
                REQUIRE(var.get_initializer() == "");
                REQUIRE(var.get_linkage() == cpp_no_linkage);
                REQUIRE(!dynamic_cast<const cpp_member_variable&>(var).is_mutable());
                REQUIRE(dynamic_cast<const cpp_bitfield&>(var).no_bits() == 0);
            }
            else if (var.get_name() == "h")
            {
                ++count;
                REQUIRE(var.get_type().get_name() == "int");
                REQUIRE(var.get_initializer() == "");
                REQUIRE(var.get_linkage() == cpp_no_linkage);
                REQUIRE(dynamic_cast<const cpp_member_variable&>(var).is_mutable());
                REQUIRE(dynamic_cast<const cpp_bitfield&>(var).no_bits() == 1);
            }
            else
                REQUIRE(false);
        }
    });
    REQUIRE(count == 9);
}
