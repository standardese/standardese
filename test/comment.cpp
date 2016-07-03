// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment.hpp>

#include <catch.hpp>

#include <standardese/md_blocks.hpp>
#include <standardese/md_inlines.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

std::string get_text(const md_paragraph& paragraph)
{
    REQUIRE(paragraph.begin()->get_entity_type() == md_entity::text_t);
    return dynamic_cast<const md_text&>(*paragraph.begin()).get_string();
}

TEST_CASE("comment", "[doc]")
{
    parser p;

    SECTION("simple parsing")
    {
        auto comment = comment::parse(p, "", R"(/// Hello World.)");
        auto count   = 0u;
        for (auto& child : comment.get_document())
        {
            REQUIRE(child.get_entity_type() == md_entity::paragraph_t);

            auto& paragraph = dynamic_cast<const md_paragraph&>(child);
            REQUIRE(get_text(paragraph) == std::string("Hello World."));
            REQUIRE(paragraph.get_section_type() == section_type::brief);
            ++count;
        }
        REQUIRE(count == 1u);
    }
    SECTION("multiple sections explicit")
    {
        auto comment = comment::parse(p, "", R"(/// \brief A
                                                 ///
                                                 /// \details B
                                                 /// C
                                               )");

        auto count = 0u;
        for (auto& child : comment.get_document())
        {
            REQUIRE(child.get_entity_type() == md_entity::paragraph_t);
            auto& paragraph = dynamic_cast<const md_paragraph&>(child);

            if (get_text(paragraph) == "A")
            {
                ++count;
                REQUIRE(paragraph.get_section_type() == section_type::brief);
            }
            else if (get_text(paragraph) == "B")
            {
                ++count;
                REQUIRE(paragraph.get_section_type() == section_type::details);
            }
            else
                REQUIRE(false);
        }
        REQUIRE(count == 2u);
    }
    SECTION("multiple sections implicit")
    {
        auto comment = comment::parse(p, "", R"(///  A
                                                ///
                                                /// B
                                                /// C /// C
                                               )");

        auto count = 0u;
        for (auto& child : comment.get_document())
        {
            REQUIRE(child.get_entity_type() == md_entity::paragraph_t);
            auto& paragraph = dynamic_cast<const md_paragraph&>(child);

            if (get_text(paragraph) == "A")
            {
                ++count;
                REQUIRE(paragraph.get_section_type() == section_type::brief);
            }
            else if (get_text(paragraph) == "B")
            {
                ++count;
                REQUIRE(paragraph.get_section_type() == section_type::details);
            }
            else
                REQUIRE(false);
        }
        REQUIRE(count == 2u);
    }
    SECTION("cherry pick other commands")
    {
        auto comment = comment::parse(p, "", R"(/// \effects A A
                                                /// A A
                                                ///
                                                /// \returns B B
                                                ///
                                                /// \error_conditions C C)");

        auto count = 0u;
        for (auto& child : comment.get_document())
        {
            REQUIRE(child.get_entity_type() == md_entity::paragraph_t);
            auto& paragraph = dynamic_cast<const md_paragraph&>(child);
            INFO(get_text(paragraph));

            if (get_text(paragraph) == " A A")
            {
                ++count;
                REQUIRE(paragraph.get_section_type() == section_type::effects);
            }
            else if (get_text(paragraph) == " B B")
            {
                ++count;
                REQUIRE(paragraph.get_section_type() == section_type::returns);
            }
            else if (get_text(paragraph) == " C C")
            {
                ++count;
                REQUIRE(paragraph.get_section_type() == section_type::error_conditions);
            }
            else
                REQUIRE(false);
        }
        REQUIRE(count == 3u);
    }
}
