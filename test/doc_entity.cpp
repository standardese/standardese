// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/doc_entity.hpp>

#include <catch.hpp>

#include "test_parser.hpp"

using namespace standardese;

std::string debug_string(const standardese::doc_entity& entity, unsigned level = 0)
{
    std::string result(2 * level, ' ');
    if (level == 0u)
        result += '\n';

    switch (entity.kind())
    {
    case doc_entity::excluded:
        REQUIRE(entity.begin() == entity.end());
        result += "excluded";
        break;
    case doc_entity::member_group:
        result += "group";
        break;
    case doc_entity::cpp_entity:
        result += "entity";
        break;
    case doc_entity::cpp_namespace:
        result += "namespace";
        break;
    case doc_entity::cpp_file:
        result += "file";
        break;
    }

    result += " - " + entity.link_name() + '\n';
    for (auto& child : entity)
        result += debug_string(child, level + 1u);

    return result;
}

// only test building here
TEST_CASE("doc_entity")
{
    standardese::comment_registry comments;
    SECTION("basic")
    {
        auto file = build_doc_entities(comments, {}, "doc_entity__basic", R"(
using a = int;
using b = a;

namespace ns
{
   template <typename T>
   struct type
   {
      int a;
      int b;
   };

   type<int> func(b param);
}
)");

        REQUIRE(debug_string(*file) == R"(
file - doc_entity__basic
  entity - a
  entity - b
  namespace - ns
    entity - ns::type<T>
      entity - ns::type<T>.T
      entity - ns::type<T>::a
      entity - ns::type<T>::b
    entity - ns::func(b)
      entity - ns::func(b).param
)");
    }
    SECTION("ignored")
    {
        auto file = build_doc_entities(comments, {}, "doc_entity__ignored", R"(
#include <cstddef>

using namespace std;
using std::size_t;

class foo
{
public:
   static_assert(sizeof(size_t) == sizeof(size_t), "");

   friend struct bar;
   friend void func(int);

   friend void inline_friend() {}
};

extern "C" int func();
)");

        REQUIRE(debug_string(*file) == R"(
file - doc_entity__ignored
  entity - foo
    entity - foo::inline_friend()
  entity - func()
)");
    }
    SECTION("excluded")
    {
        auto file = build_doc_entities(comments, {}, "doc_entity__excluded", R"(
/// \exclude
void a();

class foo; // excluded

/// \exclude
namespace ns
{
   void b();
   void c();
}

class foo
{
   void d(); // excluded
   virtual void e();

protected:
   void f();

public:
   void g();
};

/// not excluded as it is documented
class foo;
)");

        REQUIRE(debug_string(*file) == R"(
file - doc_entity__excluded
  entity - foo
    entity - foo::e()
    entity - foo::f()
    entity - foo::g()
  entity - foo
)");
    }
    SECTION("member groups")
    {
        auto file = build_doc_entities(comments, {}, "doc_entity__member_groups", R"(
/// \group foo
void foo();

/// \group bar
void bar();

/// \group foo
void foo(int);
)");

        REQUIRE(debug_string(*file) == R"(
file - doc_entity__member_groups
  group - foo
    entity - foo()
    entity - foo(int)
      entity - foo(int).0
  group - bar
    entity - bar()
)");
    }
}
