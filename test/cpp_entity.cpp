// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_entity.hpp>

#include <catch.hpp>

using namespace standardese;

TEST_CASE("cpp_entity", "[cpp]")
{
    struct test_entity : cpp_entity
    {
        test_entity(const char *name)
        : cpp_entity("", name, "") {}
    };

    struct container : cpp_entity_container<cpp_entity>
    {
        container()
        : cpp_entity_container() {}

        // convenience
        void add_entity(test_entity *e)
        {
            cpp_entity_container::add_entity(cpp_entity_ptr(e));
        }
    };

    container container;
    REQUIRE(container.empty());
    REQUIRE(container.begin() == container.end());

    container.add_entity(new test_entity("a"));
    REQUIRE(container.begin()->get_name() == "a");
    REQUIRE(container.begin() != container.end());
    REQUIRE(std::next(container.begin()) == container.end());
    REQUIRE(!container.empty());

    container.add_entity(new test_entity("b"));
    container.add_entity(nullptr);
    container.add_entity(new test_entity("c"));
    container.add_entity(new test_entity("d"));

    auto cur = container.begin();
    REQUIRE(cur->get_name() == "a");
    cur++;
    REQUIRE(cur->get_name() == "b");
    ++cur;
    REQUIRE(cur->get_name() == "c");
    cur++;
    REQUIRE(cur->get_name() == "d");

    auto last = cur;
    REQUIRE(cur++ != container.end());
    REQUIRE(cur == container.end());

    container.add_entity(new test_entity("e"));
    REQUIRE(cur == container.end());
    ++last;
    REQUIRE(last->get_name() == "e");
    ++last;
    REQUIRE(last == container.end());
    REQUIRE(!container.empty());
}
