// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_template.hpp>

#include <catch.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("cpp_template_type_parameter", "[cpp]")
{
    parser p;

    auto code = R"(
        template <typename A, typename B = int>
        struct s1;

        template <typename ... C>
        struct s2;

        template <class ... D>
        struct s3;

        namespace ns
        {
            struct s;

            template <class E = s>
            struct s4;
        }

        template <typename F = ns::s>
        struct s5;

        template <typename G = s1<int>>
        struct s6;
    )";

    auto tu = parse(p, "cpp_template_type_parameter", code);
    auto count = 0u;
    for (auto& param : parse_entity<cpp_template_type_parameter>(tu, CXCursor_TemplateTypeParameter))
    {
        if (param->get_name() == "A")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(!param->has_default_type());
        }
        else if (param->get_name() == "B")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(param->has_default_type());
            REQUIRE(param->get_default_type().get_name() == "int");
        }
        else if (param->get_name() == "C")
        {
            ++count;
            REQUIRE(param->is_variadic());
            REQUIRE(!param->has_default_type());
        }
        else if (param->get_name() == "D")
        {
            ++count;
            REQUIRE(param->is_variadic());
            REQUIRE(!param->has_default_type());
        }
        else if (param->get_name() == "E")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(param->has_default_type());
            REQUIRE(param->get_default_type().get_name() == "s");
            // bug: REQUIRE(param->get_default_type().get_full_name() == "ns::s");
        }
        else if (param->get_name() == "F")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(param->has_default_type());
            REQUIRE(param->get_default_type().get_name() == "ns::s");
            // bug: REQUIRE(param->get_default_type().get_full_name() == "ns::s");
        }
        else if (param->get_name() == "G")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(param->has_default_type());
            REQUIRE(param->get_default_type().get_name() == "s1<int>");
            // bug: REQUIRE(param->get_default_type().get_full_name() == "s1<int, int>");
        }
        else
            REQUIRE(false);
    }
    REQUIRE(count == 7u);
}

TEST_CASE("cpp_non_type_template_parameter", "[cpp]")
{
    parser p;

    auto code = R"(
        template <int A, int B = 0>
        struct s1;

        template <int ... C>
        struct s2;

        template <const int &D>
        struct s4;

        template <int (*E)(float, ...)>
        struct s5;

        template <int (*... F)(float, ...)>
        struct s6;

        template <typename T, int G = T::template get<4>>
        struct s7;
    )";

    auto tu = parse(p, "cpp_non_type_template_parameter", code);
    auto count = 0u;
    for (auto& param : parse_entity<cpp_non_type_template_parameter>(tu, CXCursor_NonTypeTemplateParameter))
    {
        if (param->get_name() == "A")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(param->get_type().get_name() == "int");
            REQUIRE(!param->has_default_value());
        }
        else if (param->get_name() == "B")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(param->get_type().get_name() == "int");
            REQUIRE(param->has_default_value());
            REQUIRE(param->get_default_value() == "0");
        }
        else if (param->get_name() == "C")
        {
            ++count;
            REQUIRE(param->is_variadic());
            REQUIRE(param->get_type().get_name() == "int");
            REQUIRE(!param->has_default_value());
        }
        else if (param->get_name() == "D")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(param->get_type().get_name() == "const int &");
            REQUIRE(!param->has_default_value());
        }
        else if (param->get_name() == "E")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(param->get_type().get_name() == "int(*)(float, ...)");
            REQUIRE(!param->has_default_value());
        }
        else if (param->get_name() == "F")
        {
            ++count;
            REQUIRE(param->is_variadic());
            REQUIRE(param->get_type().get_name() == "int(*)(float, ...)");
            REQUIRE(!param->has_default_value());
        }
        else if (param->get_name() == "G")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(param->get_type().get_name() == "int");
            REQUIRE(param->has_default_value());
            REQUIRE(param->get_default_value() == "T::template get<4>");
        }
        else
            REQUIRE(false);
    }
    REQUIRE(count == 7u);
}

TEST_CASE("cpp_template_template_parameter", "[cpp]")
{
    parser p;

    auto code = R"(
        template <template <typename> class A>
        struct s1;

        template <typename T, int I>
        struct s2;

        template <template <typename a, int b> class B = s2>
        struct s3;

        namespace ns
        {
            template <typename, int>
            struct s;

            template <template <typename a, int b> class C = s>
            struct s4;
        }

        template <template <typename> class ... D>
        struct s5;

        template <template <int a, typename ... b> class ... E>
        struct s6;

        template <template <class a, template <typename> class b> class F>
        struct s7;
    )";

    auto tu = parse(p, "cpp_template_template_parameter", code);
    auto count = 0u;
    for (auto& param : parse_entity<cpp_template_template_parameter>(tu, CXCursor_TemplateTemplateParameter))
    {
        if (param->get_name() == "A")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(!param->has_default_template());

            auto size = 0u;
            for (auto& e : *param)
            {
                ++size;
                REQUIRE(e.get_name() == "");
                REQUIRE_NOTHROW(dynamic_cast<const cpp_template_type_parameter&>(e));
            }
            REQUIRE(size == 1u);
        }
        else if (param->get_name() == "B")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(param->has_default_template());
            REQUIRE(param->get_default_template().get_name() == "s2");
            REQUIRE(param->get_default_template().get_full_name() == "s2");

            auto size = 0u;
            for (auto& e : *param)
            {
                if (e.get_name() == "a")
                {
                    ++size;
                    REQUIRE_NOTHROW(dynamic_cast<const cpp_template_type_parameter &>(e));
                    REQUIRE(!e.is_variadic());
                }
                else if (e.get_name() == "b")
                {
                    ++size;
                    REQUIRE_NOTHROW(dynamic_cast<const cpp_non_type_template_parameter &>(e));
                    REQUIRE(!e.is_variadic());
                }
                else
                    REQUIRE(false);
            }
            REQUIRE(size == 2u);
        }
        else if (param->get_name() == "C")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(param->has_default_template());
            REQUIRE(param->get_default_template().get_name() == "s");
            REQUIRE(param->get_default_template().get_full_name() == "ns::s");

            auto size = 0u;
            for (auto& e : *param)
            {
                if (e.get_name() == "a")
                {
                    ++size;
                    REQUIRE_NOTHROW(dynamic_cast<const cpp_template_type_parameter &>(e));
                    REQUIRE(!e.is_variadic());
                }
                else if (e.get_name() == "b")
                {
                    ++size;
                    REQUIRE_NOTHROW(dynamic_cast<const cpp_non_type_template_parameter &>(e));
                    REQUIRE(!e.is_variadic());
                }
                else
                    REQUIRE(false);
            }
            REQUIRE(size == 2u);
        }
        else if (param->get_name() == "D")
        {
            ++count;
            REQUIRE(param->is_variadic());
            REQUIRE(!param->has_default_template());

            auto size = 0u;
            for (auto& e : *param)
            {
                ++size;
                REQUIRE(e.get_name() == "");
                REQUIRE_NOTHROW(dynamic_cast<const cpp_template_type_parameter&>(e));
            }
            REQUIRE(size == 1u);
        }
        else if (param->get_name() == "E")
        {
            ++count;
            REQUIRE(param->is_variadic());
            REQUIRE(!param->has_default_template());

            auto size = 0u;
            for (auto& e : *param)
            {
                if (e.get_name() == "b")
                {
                    ++size;
                    REQUIRE_NOTHROW(dynamic_cast<const cpp_template_type_parameter &>(e));
                    REQUIRE(e.is_variadic());
                }
                else if (e.get_name() == "a")
                {
                    ++size;
                    REQUIRE_NOTHROW(dynamic_cast<const cpp_non_type_template_parameter &>(e));
                    REQUIRE(!e.is_variadic());
                }
                else
                    REQUIRE(false);
            }
            REQUIRE(size == 2u);
        }
        else if (param->get_name() == "F")
        {
            ++count;
            REQUIRE(!param->is_variadic());
            REQUIRE(!param->has_default_template());

            auto size = 0u;
            for (auto& e : *param)
            {
                if (e.get_name() == "a")
                {
                    ++size;
                    REQUIRE_NOTHROW(dynamic_cast<const cpp_template_type_parameter &>(e));
                    REQUIRE(!e.is_variadic());
                }
                else if (e.get_name() == "b")
                {
                    ++size;
                    auto& te = dynamic_cast<const cpp_template_template_parameter &>(e);
                    REQUIRE(!te.is_variadic());

                    REQUIRE_NOTHROW(dynamic_cast<const cpp_template_type_parameter&>(*te.begin()));
                    REQUIRE(std::next(te.begin()) == te.end());
                }
                else
                    REQUIRE(false);
            }
            REQUIRE(size == 2u);
        }
    }
    REQUIRE(count == 6u);
}
