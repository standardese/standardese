// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/synopsis.hpp>

#include <catch.hpp>

using namespace standardese;

TEST_CASE("entity_blacklist")
{
    struct dummy_entity : cpp_entity
    {
        cpp_name name;

        dummy_entity(cpp_name name, cpp_entity::type t)
        : cpp_entity(t, cpp_cursor(), (std::unique_ptr<md_comment>())), name(std::move(name))
        {
        }

        cpp_name get_name() const override
        {
            return name;
        }
    };

    entity_blacklist blacklist(entity_blacklist::empty);

    dummy_entity ns("ns", cpp_entity::namespace_t);
    dummy_entity type("foo", cpp_entity::class_t);
    dummy_entity variable("foo", cpp_entity::variable_t);

    SECTION("none")
    {
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::documentation, ns));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::documentation, type));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::documentation, variable));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::synopsis, ns));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::synopsis, type));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::synopsis, variable));
    }
    SECTION("name-type")
    {
        blacklist.blacklist("ns", cpp_entity::namespace_t);
        REQUIRE(blacklist.is_blacklisted(entity_blacklist::documentation, ns));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::documentation, type));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::documentation, variable));
        REQUIRE(blacklist.is_blacklisted(entity_blacklist::synopsis, ns));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::synopsis, type));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::synopsis, variable));
    }
    SECTION("name")
    {
        blacklist.blacklist("foo");
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::documentation, ns));
        REQUIRE(blacklist.is_blacklisted(entity_blacklist::documentation, type));
        REQUIRE(blacklist.is_blacklisted(entity_blacklist::documentation, variable));
        REQUIRE(!blacklist.is_blacklisted(entity_blacklist::synopsis, ns));
        REQUIRE(blacklist.is_blacklisted(entity_blacklist::synopsis, type));
        REQUIRE(blacklist.is_blacklisted(entity_blacklist::synopsis, variable));
    }
    SECTION("option test")
    {
        REQUIRE(!blacklist.is_set(entity_blacklist::require_comment));
        REQUIRE(!blacklist.is_set(entity_blacklist::extract_private));

        blacklist.set_option(entity_blacklist::require_comment);
        REQUIRE(blacklist.is_set(entity_blacklist::require_comment));
        REQUIRE(!blacklist.is_set(entity_blacklist::extract_private));

        blacklist.set_option(entity_blacklist::extract_private);
        REQUIRE(blacklist.is_set(entity_blacklist::require_comment));
        REQUIRE(blacklist.is_set(entity_blacklist::extract_private));

        blacklist.unset_option(entity_blacklist::require_comment);
        REQUIRE(!blacklist.is_set(entity_blacklist::require_comment));
        REQUIRE(blacklist.is_set(entity_blacklist::extract_private));
    }
}
