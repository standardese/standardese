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
        comment::parser p("", R"(/// Hello World.)");
        auto comment = p.finish();
        auto sections = comment.get_sections();
        REQUIRE(sections.size() == 1u);

        REQUIRE(sections[0].type == section_type::brief);
        REQUIRE(sections[0].name == "");
        REQUIRE(sections[0].body == "Hello World.");
    }
    SECTION("multiple sections explicit")
    {
        comment::parser p("", R"(/// \brief A
                                 ///
                                 /// \details B
                                 /// C /// C
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
        REQUIRE(sections[2].body == "C /// C");
    }
    SECTION("multiple sections implicit")
    {
        comment::parser p("", R"(/// A
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
    SECTION("cherry pick other commands")
    {
        comment::parser p("", R"(/// \effects A A
                                 /// A A
                                 /// \returns B B
                                 /// \error_conditions C C)");

        auto comment = p.finish();
        auto sections = comment.get_sections();
        REQUIRE(sections.size() == 4u);

        REQUIRE(sections[0].type == section_type::effects);
        REQUIRE(sections[0].name == "Effects");
        REQUIRE(sections[0].body == "A A");

        REQUIRE(sections[1].type == section_type::effects);
        REQUIRE(sections[1].name == "Effects");
        REQUIRE(sections[1].body == "A A");

        REQUIRE(sections[2].type == section_type::returns);
        REQUIRE(sections[2].name == "Returns");
        REQUIRE(sections[2].body == "B B");

        REQUIRE(sections[3].type == section_type::error_conditions);
        REQUIRE(sections[3].name == "Error conditions");
        REQUIRE(sections[3].body == "C C");
    }
}
