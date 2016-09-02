// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_template.hpp>

#include <catch.hpp>
#include <standardese/detail/parse_utils.hpp>
#include <standardese/cpp_class.hpp>
#include <standardese/cpp_function.hpp>

#include "test_parser.hpp"

using namespace standardese;

TEST_CASE("cpp_template_type_parameter", "[cpp]")
{
    parser p;

    auto code = R"(
        template <typename A, typename B = decltype(0)>
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

    auto tu    = parse(p, "cpp_template_type_parameter", code);
    auto count = 0u;
    for (auto& param :
         parse_entity<cpp_template_type_parameter>(tu, CXCursor_TemplateTypeParameter))
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
            REQUIRE(param->get_default_type().get_name() == "decltype(0)");
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

        template <typename T, int G = T::template get<((1 << 4) > (3 >> (2 <= 1 && 3 < 2)))>>
        struct s7;
    )";

    auto tu    = parse(p, "cpp_non_type_template_parameter", code);
    auto count = 0u;
    for (auto& param :
         parse_entity<cpp_non_type_template_parameter>(tu, CXCursor_NonTypeTemplateParameter))
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
            REQUIRE(param->get_type().get_name() == "const int&");
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
            REQUIRE(param->get_default_value() == "T::template get<((1<<4)>(3>>(2<=1&&3<2)))>");
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

    auto tu    = parse(p, "cpp_template_template_parameter", code);
    auto count = 0u;
    for (auto& param :
         parse_entity<cpp_template_template_parameter>(tu, CXCursor_TemplateTemplateParameter))
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
                REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&e) != nullptr);
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
                    REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&e) != nullptr);
                    REQUIRE(!e.is_variadic());
                }
                else if (e.get_name() == "b")
                {
                    ++size;
                    REQUIRE(dynamic_cast<const cpp_non_type_template_parameter*>(&e) != nullptr);
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
                    REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&e) != nullptr);
                    REQUIRE(!e.is_variadic());
                }
                else if (e.get_name() == "b")
                {
                    ++size;
                    REQUIRE(dynamic_cast<const cpp_non_type_template_parameter*>(&e) != nullptr);
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
                REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&e) != nullptr);
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
                    REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&e) != nullptr);
                    REQUIRE(e.is_variadic());
                }
                else if (e.get_name() == "a")
                {
                    ++size;
                    REQUIRE(dynamic_cast<const cpp_non_type_template_parameter*>(&e) != nullptr);
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
                    REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&e) != nullptr);
                    REQUIRE(!e.is_variadic());
                }
                else if (e.get_name() == "b")
                {
                    ++size;
                    auto& te = dynamic_cast<const cpp_template_template_parameter&>(e);
                    REQUIRE(!te.is_variadic());

                    REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&*te.begin())
                            != nullptr);
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

// just a quick test to see whether the correct type is unwrapped
// parsing just forwards
TEST_CASE("cpp_function_template and specialization", "[cpp]")
{
    parser p;

    auto code = R"(
        /// a
        template <typename A>
        void a(A t);

        /// a
        template <>
        void a<int>(int t);

        /// b
        template <int A, typename B>
        B (*b(B t))();

        /// b
        template <>
        int* (*b<0, int*>(int* t))();

        /// c
        template <int val = ((1 << 4) > (3 >> (2 <= 1 && 3 < 2)))>
        void c();

        /// c
        template <>
        void c<-((1 << 4) > (3 >> (2 <= 1 && 3 < 2)))>()
        {
            int a = {};
            if (a)
                ::a(a);
        }


        /// d
        template <bool A = 0 < 1>
        void d();

        /// d
        template <>
        void d<0 < -1>();

        struct foo
        {
            /// c
            template <typename ... A>
            void c(A ... a);

            /// foo
            template <typename A>
            foo(A a);

            /// operator
            template <typename T>
            operator T*();
        };
    )";

    auto tu = parse(p, "cpp_function_template", code);

    auto count = 0u;
    for_each(tu.get_file(), [&](const cpp_entity& e) {
        if (auto ptr = dynamic_cast<const cpp_function_template*>(&e))
        {
            if (ptr->get_function().get_name() == "a")
            {
                ++count;
                REQUIRE(ptr->get_name() == "a<A>");
                REQUIRE(detail::parse_comment(ptr->get_cursor()) == "/// a");
                REQUIRE(ptr->get_signature() == "(A)");
                auto& func = dynamic_cast<const cpp_function&>(ptr->get_function());
                REQUIRE(func.get_return_type().get_name() == "void");

                auto size = 0u;
                for (auto& param : ptr->get_template_parameters())
                {
                    if (param.get_name() == "A")
                    {
                        ++size;
                        REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&param)
                                != nullptr);
                        REQUIRE(!param.is_variadic());
                    }
                    else
                        REQUIRE(false);
                }
                REQUIRE(size == 1u);
            }
            else if (ptr->get_function().get_name() == "b")
            {
                ++count;
                REQUIRE(ptr->get_name() == "b<A, B>");
                REQUIRE(detail::parse_comment(ptr->get_cursor()) == "/// b");
                REQUIRE(ptr->get_signature() == "(B)");
                auto& func = dynamic_cast<const cpp_function&>(ptr->get_function());
                REQUIRE(func.get_return_type().get_name() == "B(*)()");

                auto size = 0u;
                for (auto& param : ptr->get_template_parameters())
                {
                    if (param.get_name() == "A")
                    {
                        ++size;
                        REQUIRE(dynamic_cast<const cpp_non_type_template_parameter*>(&param)
                                != nullptr);
                        REQUIRE(!param.is_variadic());
                    }
                    else if (param.get_name() == "B")
                    {
                        ++size;
                        REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&param)
                                != nullptr);
                        REQUIRE(!param.is_variadic());
                    }
                    else
                        REQUIRE(false);
                }
                REQUIRE(size == 2u);
            }
            else if (ptr->get_function().get_name() == "c")
            {
                ++count;
                REQUIRE(ptr->get_name() == "c<val>");
                REQUIRE(detail::parse_comment(ptr->get_cursor()) == "/// c");
                REQUIRE(ptr->get_signature() == "()");
                auto& func = dynamic_cast<const cpp_function&>(ptr->get_function());
                REQUIRE(func.get_return_type().get_name() == "void");

                auto size = 0u;
                for (auto& param : ptr->get_template_parameters())
                {
                    if (param.get_name() == "val")
                    {
                        ++size;
                        REQUIRE(dynamic_cast<const cpp_non_type_template_parameter*>(&param)
                                != nullptr);
                        REQUIRE(!param.is_variadic());
                    }
                    else
                        REQUIRE(false);
                }
                REQUIRE(size == 1u);
            }
            else if (ptr->get_function().get_name() == "d")
            {
                ++count;
                REQUIRE(ptr->get_name() == "d<A>");
                REQUIRE(detail::parse_comment(ptr->get_cursor()) == "/// d");
                REQUIRE(ptr->get_signature() == "()");
                auto& func = dynamic_cast<const cpp_function&>(ptr->get_function());
                REQUIRE(func.get_return_type().get_name() == "void");

                auto size = 0u;
                for (auto& param : ptr->get_template_parameters())
                {
                    if (param.get_name() == "A")
                    {
                        ++size;
                        REQUIRE(dynamic_cast<const cpp_non_type_template_parameter*>(&param)
                                != nullptr);
                        REQUIRE(!param.is_variadic());
                    }
                    else
                        REQUIRE(false);
                }
                REQUIRE(size == 1u);
            }
            else
                REQUIRE(false);
        }
        else if (auto ptr = dynamic_cast<const cpp_function_template_specialization*>(&e))
        {
            if (ptr->get_function().get_name() == "a")
            {
                ++count;
                REQUIRE(ptr->get_name() == "a<int>");
                REQUIRE(detail::parse_comment(ptr->get_cursor()) == "/// a");
                REQUIRE(ptr->get_signature() == "(int)");
                auto& func = dynamic_cast<const cpp_function&>(ptr->get_function());
                REQUIRE(func.get_return_type().get_name() == "void");
            }
            else if (ptr->get_function().get_name() == "b")
            {
                ++count;
                REQUIRE(ptr->get_name() == "b<0, int*>");
                REQUIRE(detail::parse_comment(ptr->get_cursor()) == "/// b");
                REQUIRE(ptr->get_signature() == "(int *)");
                auto& func = dynamic_cast<const cpp_function&>(ptr->get_function());
                REQUIRE(func.get_return_type().get_name() == "int*(*)()");
            }
            else if (ptr->get_function().get_name() == "c")
            {
                ++count;
                REQUIRE(ptr->get_name() == "c<-((1<<4)>(3>>(2<=1&&3<2)))>");
                REQUIRE(detail::parse_comment(ptr->get_cursor()) == "/// c");
                REQUIRE(ptr->get_signature() == "()");
                auto& func = dynamic_cast<const cpp_function&>(ptr->get_function());
                REQUIRE(func.get_return_type().get_name() == "void");
            }
            else if (ptr->get_function().get_name() == "d")
            {
                ++count;
                REQUIRE(ptr->get_name() == "d<0<-1>");
                REQUIRE(detail::parse_comment(ptr->get_cursor()) == "/// d");
                REQUIRE(ptr->get_signature() == "()");
                auto& func = dynamic_cast<const cpp_function&>(ptr->get_function());
                REQUIRE(func.get_return_type().get_name() == "void");
            }
            else
                REQUIRE(false);
        }
        else if (auto ptr = dynamic_cast<const cpp_class*>(&e))
        {
            for (auto& e : *ptr)
            {
                if (auto ptr = dynamic_cast<const cpp_function_template*>(&e))
                {
                    if (ptr->get_function().get_name() == "c")
                    {
                        ++count;
                        REQUIRE(detail::parse_comment(ptr->get_cursor()) == "/// c");
                        REQUIRE(dynamic_cast<const cpp_member_function*>(&ptr->get_function())
                                != nullptr);
                        REQUIRE(!ptr->get_function().is_variadic());
                        REQUIRE(ptr->get_name() == "c<A...>");
                        REQUIRE(ptr->get_signature() == "(A...)");

                        auto size = 0u;
                        for (auto& param : ptr->get_template_parameters())
                        {
                            if (param.get_name() == "A")
                            {
                                ++size;
                                REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&param)
                                        != nullptr);
                                REQUIRE(param.is_variadic());
                            }
                            else
                                REQUIRE(false);
                        }
                        REQUIRE(size == 1u);
                    }
                    else if (ptr->get_function().get_name() == "foo")
                    {
                        ++count;
                        REQUIRE(detail::parse_comment(ptr->get_cursor()) == "/// foo");
                        REQUIRE(dynamic_cast<const cpp_constructor*>(&ptr->get_function())
                                != nullptr);
                        REQUIRE(ptr->get_name() == "foo<A>");
                        REQUIRE(ptr->get_signature() == "(A)");

                        auto size = 0u;
                        for (auto& param : ptr->get_template_parameters())
                        {
                            if (param.get_name() == "A")
                            {
                                ++size;
                                REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&param)
                                        != nullptr);
                                REQUIRE(!param.is_variadic());
                            }
                            else
                                REQUIRE(false);
                        }
                        REQUIRE(size == 1u);
                    }
                    else if (ptr->get_function().get_name() == "operator T *")
                    {
                        ++count;
                        REQUIRE(detail::parse_comment(ptr->get_cursor()) == "/// operator");
                        REQUIRE(dynamic_cast<const cpp_conversion_op*>(&ptr->get_function())
                                != nullptr);
                        REQUIRE(ptr->get_name() == "operator T *<T>");
                        REQUIRE(ptr->get_signature() == "()");

                        auto size = 0u;
                        for (auto& param : ptr->get_template_parameters())
                        {
                            if (param.get_name() == "T")
                            {
                                ++size;
                                REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&param)
                                        != nullptr);
                                REQUIRE(!param.is_variadic());
                            }
                            else
                                REQUIRE(false);
                        }
                        REQUIRE(size == 1u);
                    }
                    else
                        REQUIRE(false);
                }
                else
                    REQUIRE(false);
            }
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 11u);
}

TEST_CASE("cpp_class_template", "[cpp]")
{
    parser p;

    auto code = R"(
        template <typename T>
        struct should_be_ignored;

        /// a
        template <typename A>
        struct a
        {};

        /// b
        template <int A, typename ... B>
        class b
        {};

        /// c
        template <int A = ((1 << 4) > (3 >> (2 <= 1 && 3 < 2)))>
        class c : public b<A>
        {};

        template <>
        struct should_be_ignored<int>;

        /// a
        template <>
        struct a<int *>
        {
            /// a
            a(const a &a) = default;

            /// b
            a(const a<int> &a);
        };

        /// b
        template <>
        class b<0, char, int(*)(char)> {};

        template <typename T>
        struct should_be_ignored<T*>;

        /// a
        template <class T>
        struct a<T(*)()>
        {
            /// a
            a(const a &a) = default;

            /// b
            a(const a<int *> &a);
        };

        /// b
        template <int A, typename ... B>
        class b<A, void(*)(B...)> {};

        /// d
        template <int A = 0 < 1>
        class d {};

        /// d
        template <>
        class d<1 < 0> {};
    )";

    auto tu = parse(p, "cpp_class_template", code);

    auto count = 0u;
    for_each(tu.get_file(), [&](const cpp_entity& e) {
        if (auto c = dynamic_cast<const cpp_class_template*>(&e))
        {
            if (c->get_class().get_name() == "a")
            {
                ++count;
                REQUIRE(c->get_name() == "a<A>");
                REQUIRE(detail::parse_comment(c->get_cursor()) == "/// a");
                REQUIRE(c->get_class().get_class_type() == cpp_struct_t);

                auto size = 0u;
                for (auto& param : c->get_template_parameters())
                {
                    if (param.get_name() == "A")
                    {
                        ++size;
                        REQUIRE(param.get_unique_name() == "a<A>.A");
                        REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&param)
                                != nullptr);
                        REQUIRE(!param.is_variadic());
                    }
                    else
                        REQUIRE(false);
                }
                REQUIRE(size == 1u);
            }
            else if (c->get_class().get_name() == "b")
            {
                ++count;
                REQUIRE(c->get_name() == "b<A, B...>");
                REQUIRE(detail::parse_comment(c->get_cursor()) == "/// b");
                REQUIRE(c->get_class().get_class_type() == cpp_class_t);

                auto size = 0u;
                for (auto& param : c->get_template_parameters())
                {
                    if (param.get_name() == "A")
                    {
                        ++size;
                        REQUIRE(param.get_unique_name() == "b<A, B...>.A");
                        REQUIRE(dynamic_cast<const cpp_non_type_template_parameter*>(&param)
                                != nullptr);
                        REQUIRE(!param.is_variadic());
                    }
                    else if (param.get_name() == "B")
                    {
                        ++size;
                        REQUIRE(param.get_unique_name() == "b<A, B...>.B");
                        REQUIRE(dynamic_cast<const cpp_template_type_parameter*>(&param)
                                != nullptr);
                        REQUIRE(param.is_variadic());
                    }
                    else
                        REQUIRE(false);
                }
                REQUIRE(size == 2u);
            }
            else if (c->get_class().get_name() == "c")
            {
                ++count;
                REQUIRE(c->get_name() == "c<A>");
                REQUIRE(detail::parse_comment(c->get_cursor()) == "/// c");
                REQUIRE(c->get_class().get_class_type() == cpp_class_t);

                auto size = 0u;
                for (auto& param : c->get_template_parameters())
                {
                    if (param.get_name() == "A")
                    {
                        ++size;
                        REQUIRE(param.get_unique_name() == "c<A>.A");
                        REQUIRE(dynamic_cast<const cpp_non_type_template_parameter*>(&param)
                                != nullptr);
                        REQUIRE(!param.is_variadic());
                    }
                    else
                        REQUIRE(false);
                }
                REQUIRE(size == 1u);

                for (auto& base : c->get_class().get_bases())
                {
                    REQUIRE(base.get_name() == "b<A>");
                    REQUIRE(base.get_unique_name() == "c<A>::b<A>");
                    REQUIRE(base.get_access() == cpp_access_specifier_t::cpp_public);
                }
            }
            else if (c->get_class().get_name() == "d")
            {
                ++count;
                REQUIRE(c->get_name() == "d<A>");
                REQUIRE(detail::parse_comment(c->get_cursor()) == "/// d");
                REQUIRE(c->get_class().get_class_type() == cpp_class_t);

                auto size = 0u;
                for (auto& param : c->get_template_parameters())
                {
                    if (param.get_name() == "A")
                    {
                        ++size;
                        REQUIRE(param.get_unique_name() == "d<A>.A");
                        REQUIRE(dynamic_cast<const cpp_non_type_template_parameter*>(&param)
                                != nullptr);
                        REQUIRE(!param.is_variadic());
                    }
                    else
                        REQUIRE(false);
                }
                REQUIRE(size == 1u);
            }
            else
                REQUIRE(false);
        }
        else if (auto c = dynamic_cast<const cpp_class_template_full_specialization*>(&e))
        {
            if (c->get_class().get_name() == "a")
            {
                ++count;
                REQUIRE(c->get_name() == "a<int*>");
                REQUIRE(detail::parse_comment(c->get_cursor()) == "/// a");
                REQUIRE(c->get_primary_template().get_name() == "a");
                REQUIRE(c->get_class().get_class_type() == cpp_struct_t);

                for (auto& e : c->get_class())
                {
                    auto& ctor = dynamic_cast<const cpp_constructor&>(e);
                    REQUIRE(ctor.get_name() == "a");
                    for (auto& param : ctor.get_parameters())
                    {
                        REQUIRE(param.get_name() == "a");
                        if (detail::parse_comment(ctor.get_cursor()) == "/// a")
                            REQUIRE(param.get_type().get_full_name() == "const a<int *> &");
                        else if (detail::parse_comment(ctor.get_cursor()) == "/// b")
                            REQUIRE(param.get_type().get_full_name() == "const a<int> &");
                        else
                            REQUIRE(false);
                    }
                }
            }
            else if (c->get_class().get_name() == "b")
            {
                ++count;
                REQUIRE(c->get_name() == "b<0, char, int(*)(char)>");
                REQUIRE(detail::parse_comment(c->get_cursor()) == "/// b");
                REQUIRE(c->get_primary_template().get_name() == "b");
                REQUIRE(c->get_class().get_class_type() == cpp_class_t);
            }
            else if (c->get_class().get_name() == "d")
            {
                ++count;
                REQUIRE(c->get_name() == "d<1<0>");
                REQUIRE(detail::parse_comment(c->get_cursor()) == "/// d");
                REQUIRE(c->get_primary_template().get_name() == "d");
                REQUIRE(c->get_class().get_class_type() == cpp_class_t);
            }
            else
                REQUIRE(false);
        }
        else if (auto c = dynamic_cast<const cpp_class_template_partial_specialization*>(&e))
        {
            if (c->get_class().get_name() == "a")
            {
                ++count;
                REQUIRE(c->get_name() == "a<T(*)()>");
                REQUIRE(detail::parse_comment(c->get_cursor()) == "/// a");
                REQUIRE(c->get_primary_template().get_name() == "a");
                REQUIRE(c->get_class().get_class_type() == cpp_struct_t);

                for (auto& e : c->get_class())
                {
                    auto& ctor = dynamic_cast<const cpp_constructor&>(e);
                    REQUIRE(ctor.get_name() == "a");
                    for (auto& param : ctor.get_parameters())
                    {
                        REQUIRE(param.get_name() == "a");
                        if (detail::parse_comment(ctor.get_cursor()) == "/// a")
                            REQUIRE(param.get_type().get_full_name() == "const a &");
                        else if (detail::parse_comment(ctor.get_cursor()) == "/// b")
                            REQUIRE(param.get_type().get_full_name() == "const a<int *> &");
                        else
                            REQUIRE(false);
                    }
                }
            }
            else if (c->get_class().get_name() == "b")
            {
                ++count;
                REQUIRE(c->get_name() == "b<A, void(*)(B...)>");
                REQUIRE(detail::parse_comment(c->get_cursor()) == "/// b");
                REQUIRE(c->get_primary_template().get_name() == "b");
                REQUIRE(c->get_class().get_class_type() == cpp_class_t);
            }
            else
                REQUIRE(false);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 9u);
}

TEST_CASE("cpp_alias_template", "[cpp]")
{
    parser p;

    auto code = R"(
            /// a
            template <typename T>
            using a = T;

            template <typename T>
            struct foo;

            /// b
            template <typename T>
            using b = foo<T>;

            /// c
            template <typename T>
            using c = unsigned int;
        )";

    auto tu = parse(p, "cpp_alias_template", code);

#if CINDEX_VERSION_MINOR >= 32
    auto count = 0u;
    for_each(tu.get_file(), [&](const cpp_entity& e) {
        auto& alias = dynamic_cast<const cpp_alias_template&>(e);
        auto& type  = alias.get_type();

        if (alias.get_name() == "a<T>")
        {
            ++count;
            REQUIRE(detail::parse_comment(alias.get_cursor()) == "/// a");
            REQUIRE(type.get_name() == "a");
            REQUIRE(type.get_target().get_name() == "T");
            REQUIRE(type.get_target().get_full_name() == "T");
        }
        else if (alias.get_name() == "b<T>")
        {
            ++count;
            REQUIRE(detail::parse_comment(alias.get_cursor()) == "/// b");
            REQUIRE(type.get_name() == "b");
            REQUIRE(type.get_target().get_name() == "foo<T>");
            REQUIRE(type.get_target().get_full_name() == "foo<T>");
        }
        else if (alias.get_name() == "c<T>")
        {
            ++count;
            REQUIRE(detail::parse_comment(alias.get_cursor()) == "/// c");
            REQUIRE(type.get_name() == "c");
            REQUIRE(type.get_target().get_name() == "unsigned int");
            REQUIRE(type.get_target().get_full_name() == "unsigned int");
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 3u);
#endif
}
