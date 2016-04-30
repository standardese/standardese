// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_function.hpp>

#include <catch.hpp>
#include <standardese/cpp_class.hpp>

#include "test_parser.hpp"

using namespace standardese;

std::size_t no_parameters(const cpp_function_base &base)
{
    std::size_t result = 0;
    for (auto& e : base)
        ++result;
    return result;
}

TEST_CASE("cpp_function and cpp_member_function", "[cpp]")
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

        struct base
        {
            static void j();

            virtual int& k() const = 0;

            void l() volatile && = delete;

            virtual void m(int a = h()) {}
        };

        struct derived : base
        {
            virtual int& k() const override;

            void m(int a = h()) final;

            derived& operator   =(const derived &a) = default;
        };
    )";

    auto tu = parse(p, "cpp_function", code);
    auto f = tu.parse();

    // no need to check the parameters, same code as for variables
    auto count = 0u;
    p.for_each_in_namespace([&](const cpp_entity &e)
    {
        if (dynamic_cast<const cpp_function*>(&e))
        {
            auto &func = dynamic_cast<const cpp_function &>(e);
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
                REQUIRE(no_parameters(func) == 2u);
            }
            else if (func.get_name() == "b")
            {
                ++count;
                REQUIRE(func.get_return_type().get_name() == "int");
                REQUIRE(!func.is_constexpr());
                REQUIRE(func.is_variadic());
                REQUIRE(func.get_noexcept() == "false");
                REQUIRE(no_parameters(func) == 1u);
            }
            else if (func.get_name() == "c")
            {
                ++count;
                REQUIRE(func.get_return_type().get_name() == "int *");
                REQUIRE(!func.is_constexpr());
                REQUIRE(!func.is_variadic());
                REQUIRE(func.get_noexcept() == "false");
                REQUIRE(no_parameters(func) == 1u);
            }
            else if (func.get_name() == "d")
            {
                ++count;
                REQUIRE(func.get_return_type().get_name() == "char &");
                REQUIRE(!func.is_constexpr());
                REQUIRE(!func.is_variadic());
                REQUIRE(func.get_noexcept() == "true");
                REQUIRE(no_parameters(func) == 0u);
            }
            else if (func.get_name() == "e")
            {
                ++count;
                REQUIRE(func.get_return_type().get_name() == "const int");
                REQUIRE(!func.is_constexpr());
                REQUIRE(!func.is_variadic());
                REQUIRE(func.get_noexcept() == "false");
                REQUIRE(no_parameters(func) == 0u);
            }
            else if (func.get_name() == "f")
            {
                ++count;
                REQUIRE(func.get_return_type().get_name() == "int(*)(volatile char &&)");
                REQUIRE(!func.is_constexpr());
                REQUIRE(!func.is_variadic());
                REQUIRE(func.get_noexcept() == "false");
                REQUIRE(no_parameters(func) == 1u);
            }
            else if (func.get_name() == "g")
            {
                ++count;
                REQUIRE(func.get_return_type().get_name() == "const char &&");
                REQUIRE(func.is_constexpr());
                REQUIRE(!func.is_variadic());
                REQUIRE(func.get_noexcept() == "false");
                REQUIRE(no_parameters(func) == 0u);
            }
            else if (func.get_name() == "h")
            {
                ++count;
                REQUIRE(func.get_return_type().get_name() == "auto");
                REQUIRE(!func.is_constexpr());
                REQUIRE(!func.is_variadic());
                REQUIRE(func.get_noexcept() == "noexcept(e())");
                REQUIRE(no_parameters(func) == 0u);
            }
            else if (func.get_name() == "i")
            {
                ++count;
                REQUIRE(func.get_return_type().get_name() == "decltype(auto)");
                REQUIRE(!func.is_constexpr());
                REQUIRE(!func.is_variadic());
                REQUIRE(func.get_noexcept() == "false");
                REQUIRE(no_parameters(func) == 0u);
            }
            else
                REQUIRE(false);
        }
        else if (e.get_name() == "base")
        {
            auto& c = dynamic_cast<const cpp_class&>(e);
            for (auto& ent : c)
            {
                auto& func = dynamic_cast<const cpp_function&>(ent);
                REQUIRE(!func.is_constexpr());
                REQUIRE(!func.is_variadic());
                REQUIRE(func.get_noexcept() == "false");

                if (func.get_name() == "j")
                {
                    ++count;
                    REQUIRE(func.get_return_type().get_name() == "void");
                    REQUIRE(no_parameters(func) == 0u);
                }
                else
                {
                    auto& mfunc = dynamic_cast<const cpp_member_function&>(ent);
                    if (mfunc.get_name() == "k")
                    {
                        ++count;
                        REQUIRE(mfunc.get_return_type().get_name() == "int &");
                        REQUIRE(mfunc.get_virtual() == cpp_virtual_pure);
                        REQUIRE(is_const(mfunc.get_cv()));
                        REQUIRE(!is_volatile(mfunc.get_cv()));
                        REQUIRE(mfunc.get_ref_qualifier() == cpp_ref_none);
                        REQUIRE(mfunc.get_definition() == cpp_function_definition_normal);
                        REQUIRE(no_parameters(func) == 0u);
                    }
                    else if (mfunc.get_name() == "l")
                    {
                        ++count;
                        REQUIRE(mfunc.get_return_type().get_name() == "void");
                        REQUIRE(mfunc.get_virtual() == cpp_virtual_none);
                        REQUIRE(!is_const(mfunc.get_cv()));
                        REQUIRE(is_volatile(mfunc.get_cv()));
                        REQUIRE(mfunc.get_ref_qualifier() == cpp_ref_rvalue);
                        REQUIRE(mfunc.get_definition() == cpp_function_definition_deleted);
                        REQUIRE(no_parameters(func) == 0u);
                    }
                    else if (mfunc.get_name() == "m")
                    {
                        ++count;
                        REQUIRE(mfunc.get_return_type().get_name() == "void");
                        REQUIRE(mfunc.get_virtual() == cpp_virtual_new);
                        REQUIRE(!is_const(mfunc.get_cv()));
                        REQUIRE(!is_volatile(mfunc.get_cv()));
                        REQUIRE(mfunc.get_ref_qualifier() == cpp_ref_none);
                        REQUIRE(mfunc.get_definition() == cpp_function_definition_normal);
                        REQUIRE(no_parameters(func) == 1u);
                    }
                    else
                        REQUIRE(false);
                }
            }
        }
        else if (e.get_name() == "derived")
        {
            auto& c = dynamic_cast<const cpp_class&>(e);
            for (auto& ent : c)
            {
                if (ent.get_name() == "base")
                    continue; // skip access specifier

                auto& func = dynamic_cast<const cpp_member_function&>(ent);
                REQUIRE(!func.is_constexpr());
                REQUIRE(!func.is_variadic());
                REQUIRE(func.get_noexcept() == "false");

                if (func.get_name() == "k")
                {
                    ++count;
                    REQUIRE(func.get_return_type().get_name() == "int &");
                    REQUIRE(func.get_virtual() == cpp_virtual_overriden);
                    REQUIRE(is_const(func.get_cv()));
                    REQUIRE(!is_volatile(func.get_cv()));
                    REQUIRE(func.get_ref_qualifier() == cpp_ref_none);
                    REQUIRE(func.get_definition() == cpp_function_definition_normal);
                    REQUIRE(no_parameters(func) == 0u);
                }
                else if (func.get_name() == "m")
                {
                    ++count;
                    REQUIRE(func.get_return_type().get_name() == "void");
                    REQUIRE(func.get_virtual() == cpp_virtual_final);
                    REQUIRE(!is_const(func.get_cv()));
                    REQUIRE(!is_volatile(func.get_cv()));
                    REQUIRE(func.get_ref_qualifier() == cpp_ref_none);
                    REQUIRE(func.get_definition() == cpp_function_definition_normal);
                    REQUIRE(no_parameters(func) == 1u);
                }
                else if (func.get_name() == "operator=")
                {
                    ++count;
                    REQUIRE(func.get_return_type().get_name() == "derived &");
                    REQUIRE(func.get_virtual() == cpp_virtual_none);
                    REQUIRE(!is_const(func.get_cv()));
                    REQUIRE(!is_volatile(func.get_cv()));
                    REQUIRE(func.get_ref_qualifier() == cpp_ref_none);
                    REQUIRE(func.get_definition() == cpp_function_definition_defaulted);
                    REQUIRE(no_parameters(func) == 1u);
                }
                else
                    REQUIRE(false);
            }
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 16u);
}

TEST_CASE("cpp_conversion_op", "[cpp]")
{
    parser p;

    auto code = R"(
        struct foo
        {
            operator    int(); // multiple whitespace

            explicit operator const char&();

            constexpr explicit operator char() const volatile && noexcept;
        };
    )";

    auto tu = parse(p, "cpp_conversion_op", code);
    auto f = tu.parse();
    auto count = 0u;
    p.for_each_type([&](const cpp_type &t)
    {
        for (auto& e : dynamic_cast<const cpp_class&>(t))
        {
            auto& op = dynamic_cast<const cpp_conversion_op&>(e);
            REQUIRE(no_parameters(op) == 0u);

            if (op.get_name() == "operator int")
            {
                ++count;
                REQUIRE(op.get_target_type().get_name() == "int");
                REQUIRE(!op.is_explicit());
                REQUIRE(!op.is_constexpr());
                REQUIRE(!is_const(op.get_cv()));
                REQUIRE(!is_volatile(op.get_cv()));
                REQUIRE(op.get_ref_qualifier() == cpp_ref_none);
                REQUIRE(op.get_noexcept() == "false");
            }
            else if (op.get_name() == "operator const char &")
            {
                ++count;
                REQUIRE(op.get_target_type().get_name() == "const char &");
                REQUIRE(op.is_explicit());
                REQUIRE(!op.is_constexpr());
                REQUIRE(!is_const(op.get_cv()));
                REQUIRE(!is_volatile(op.get_cv()));
                REQUIRE(op.get_ref_qualifier() == cpp_ref_none);
                REQUIRE(op.get_noexcept() == "false");
            }
            else if (op.get_name() == "operator char")
            {
                ++count;
                REQUIRE(op.get_target_type().get_name() == "char");
                REQUIRE(op.is_explicit());
                REQUIRE(op.is_constexpr());
                REQUIRE(is_const(op.get_cv()));
                REQUIRE(is_volatile(op.get_cv()));
                REQUIRE(op.get_ref_qualifier() == cpp_ref_rvalue);
                REQUIRE(op.get_noexcept() == "true");
            }
            else
                REQUIRE(false);
        }
    });
    REQUIRE(count == 3u);
}

TEST_CASE("cpp_constructor", "[cpp]")
{
    parser p;

    auto code = R"(
        struct foo
        {
            /// a
            foo() = delete;

            /// b
            explicit foo(int a = {});

            /// c
            constexpr foo(char c) noexcept;

            /// d
            foo(const foo &other) = default;
        };
    )";

    auto tu = parse(p, "cpp_conversion_op", code);
    auto f = tu.parse();
    auto count = 0u;
    p.for_each_type([&](const cpp_type &t)
    {
        for (auto& e : dynamic_cast<const cpp_class&>(t))
        {
            auto& ctor = dynamic_cast<const cpp_constructor&>(e);
            REQUIRE(!ctor.is_variadic());

            if (ctor.get_comment() == "/// a")
            {
                ++count;
                REQUIRE(no_parameters(ctor) == 0u);
                REQUIRE(!ctor.is_constexpr());
                REQUIRE(!ctor.is_explicit());
                REQUIRE(ctor.get_noexcept() == "false");
                REQUIRE(ctor.get_definition() == cpp_function_definition_deleted);
            }
            else if (ctor.get_comment() == "/// b")
            {
                ++count;
                REQUIRE(no_parameters(ctor) == 1u);
                REQUIRE(!ctor.is_constexpr());
                REQUIRE(ctor.is_explicit());
                REQUIRE(ctor.get_noexcept() == "false");
                REQUIRE(ctor.get_definition() == cpp_function_definition_normal);
            }
            else if (ctor.get_comment() == "/// c")
            {
                ++count;
                REQUIRE(no_parameters(ctor) == 1u);
                REQUIRE(ctor.is_constexpr());
                REQUIRE(!ctor.is_explicit());
                REQUIRE(ctor.get_noexcept() == "true");
                REQUIRE(ctor.get_definition() == cpp_function_definition_normal);
            }
            else if (ctor.get_comment() == "/// d")
            {
                ++count;
                REQUIRE(no_parameters(ctor) == 1u);
                REQUIRE(!ctor.is_constexpr());
                REQUIRE(!ctor.is_explicit());
                REQUIRE(ctor.get_noexcept() == "false");
                REQUIRE(ctor.get_definition() == cpp_function_definition_defaulted);
            }
            else
                REQUIRE(false);
        }
    });
    REQUIRE(count == 4u);
}
