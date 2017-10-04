// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment.hpp>

#include <catch.hpp>
#include <fstream>

#include <cppast/cpp_class.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_template.hpp>
#include <cppast/visitor.hpp>

#include "test_parser.hpp"

using namespace standardese;

template <class Container>
void test_comments(const comment_registry& registry, const Container& container)
{
    for (auto& entity : container)
    {
        auto comment = registry.get_comment(entity);
        REQUIRE(comment.has_value());
        if (entity.name().empty())
            REQUIRE(!comment.value().metadata().module());
        else
        {
            REQUIRE(comment.value().metadata().module() == entity.name());
        }
    }
}

TEST_CASE("comment")
{
    SECTION("matched comments")
    {
        auto file = parse_file({}, "comment_matched_comments.cpp", R"(
/// \module a
using a = int;

/// \module b
struct b {};

/// \module c
enum class c
{
    d, //< \module d
    e, //< \module e
};
)");

        file_comment_parser parser(test_logger());
        parser.parse(type_safe::ref(*file));
        test_comments(parser.finish(), *file);
    }
    SECTION("param")
    {
        auto file = parse_file({}, "comment_param.cpp", R"(
/// \module a
///
/// \param b
/// \module b
///
/// \param c
/// \module c
///
/// \param 2
void a(int b, int c, int);
)");

        file_comment_parser parser(test_logger());
        parser.parse(type_safe::ref(*file));
        test_comments(parser.finish(),
                      static_cast<const cppast::cpp_function_base&>(*file->begin()).parameters());
    }
    SECTION("tparam")
    {
        auto file = parse_file({}, "comment_tparam.cpp", R"(
/// \module a
///
/// \tparam b
/// \module b
///
/// \tparam c
/// \module c
///
/// \tparam 2
template <int b, int c, int>
void a();
)");

        file_comment_parser parser(test_logger());
        parser.parse(type_safe::ref(*file));
        test_comments(parser.finish(),
                      static_cast<const cppast::cpp_template&>(*file->begin()).parameters());
    }
    SECTION("base")
    {
        auto file = parse_file({}, "comment_base.cpp", R"(
struct b {};
struct c {};

/// \module a
///
/// \base b
/// \module b
///
/// \base c
/// \module c
struct a : b, c {};
)");

        file_comment_parser parser(test_logger());
        parser.parse(type_safe::ref(*file));
        test_comments(parser.finish(),
                      static_cast<const cppast::cpp_class&>(*file->begin()).bases());
    }
    SECTION("remote")
    {
        auto file = parse_file({}, "comment_remote.cpp", R"(
/// \file
/// \module comment_remote.cpp

/// \entity a
/// \module a

/// \entity foo<T>::a
/// \module a

/// \entity foo<T>.T
/// \module T

/// \entity foo<T>::b(int, float).i
/// \module i

/// \entity foo<T>::b(int, float)
/// \module b

/// \entity custom
/// \module c

/// \entity custom::foo<int>
/// \module foo<int>

struct a {};

/// \module foo
template <typename T>
struct foo : a
{
    /// \param j
    /// \module j
    void b(int i, float j);
};

/// \unique_name custom
struct c : foo<int> {};
)");

        file_comment_parser parser(test_logger());
        parser.parse(type_safe::ref(*file));
        auto registry = parser.finish();

        auto check_comment = [&](const cppast::cpp_entity& e) {
            auto comment = registry.get_comment(e);
            INFO(e.name());
            REQUIRE(comment);
            REQUIRE(e.name() == comment.value().metadata().module());

        };

        cppast::visit(*file, [&](const cppast::cpp_entity& e, const cppast::visitor_info&) {
            check_comment(e);
            if (e.kind() == cppast::cpp_class::kind())
                for (auto& base : static_cast<const cppast::cpp_class&>(e).bases())
                    check_comment(base);
            else if (cppast::is_template(e.kind()))
                for (auto& param : static_cast<const cppast::cpp_template&>(e).parameters())
                    check_comment(param);
            else if (cppast::is_function(e.kind()))
                for (auto& param : static_cast<const cppast::cpp_function_base&>(e).parameters())
                    check_comment(param);

            return true;
        });
    }
    SECTION("member groups")
    {
        auto file = parse_file({}, "comment_member_groups.cpp", R"(
/// \group a
void a();

/// \group b Heading
void b();

/// \group a
void a(int);

/// \group b
void b(int);

/// \group c
void c();

/// \group a
void a(float);
)");

        file_comment_parser parser(test_logger());
        parser.parse(type_safe::ref(*file));
        auto groups = parser.finish();

        auto a = groups.lookup_group("a");
        REQUIRE((a.size() == 3u));
        for (auto entity : a)
            REQUIRE(entity->name() == "a");

        auto b = groups.lookup_group("b");
        REQUIRE((b.size() == 2u));
        for (auto entity : b)
            REQUIRE(entity->name() == "b");

        auto c = groups.lookup_group("c");
        REQUIRE((c.size() == 1u));
        for (auto entity : c)
            REQUIRE(entity->name() == "c");
    }
}
