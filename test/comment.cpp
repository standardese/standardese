// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment.hpp>

#include <catch.hpp>

using namespace standardese;

TEST_CASE("comment", "[doc]")
{
    SECTION("simple parsing")
    {
        comment::parser p(R"(/// Hello World.)");
        auto comment = p.finish();
        auto sections = comment.get_sections();
        REQUIRE(sections.size() == 1u);

        REQUIRE(sections[0].type == section_type::brief);
        REQUIRE(sections[0].name == "");
        REQUIRE(sections[0].body == "Hello World.");
    }
    SECTION("multiple sections explicit")
    {
        comment::parser p(R"(/// \brief A
                             ///
                             /// \details B
                             /// C
                            )");
        auto comment = p.finish();
        auto sections = comment.get_sections();
        REQUIRE(sections.size() == 3u);

        REQUIRE(sections[0].type == section_type::brief);
        REQUIRE(sections[0].name == "");
        REQUIRE(sections[0].body == "A");

        REQUIRE(sections[1].type == section_type::details);
        REQUIRE(sections[1].name == "");
        REQUIRE(sections[1].body == "B");

        REQUIRE(sections[2].type == section_type::details);
        REQUIRE(sections[2].name == "");
        REQUIRE(sections[2].body == "C");
    }
    SECTION("multiple sections implicit")
    {
        comment::parser p(R"(/// A
                             ///
                             /// B
                             /// C
                            )");
        auto comment = p.finish();
        auto sections = comment.get_sections();
        REQUIRE(sections.size() == 3u);

        REQUIRE(sections[0].type == section_type::brief);
        REQUIRE(sections[0].name == "");
        REQUIRE(sections[0].body == "A");

        REQUIRE(sections[1].type == section_type::details);
        REQUIRE(sections[1].name == "");
        REQUIRE(sections[1].body == "B");

        REQUIRE(sections[2].type == section_type::details);
        REQUIRE(sections[2].name == "");
        REQUIRE(sections[2].body == "C");
    }
}
