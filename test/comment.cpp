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
    std::string result;
    for (auto& child : paragraph)
        if (child.get_entity_type() == md_entity::text_t)
            result += dynamic_cast<const md_text&>(child).get_string();
        else if (child.get_entity_type() == md_entity::soft_break_t)
            result += '\n';
    return result;
}

std::string get_text(const md_comment& comment)
{
    REQUIRE(comment.begin()->get_entity_type() == md_entity::paragraph_t);
    return get_text(dynamic_cast<const md_paragraph&>(*comment.begin()));
}

TEST_CASE("md_comment", "[doc]")
{
    parser p;

    SECTION("comment styles")
    {
        auto comment = md_comment::parse(p, "", "/// C++ style.");
        REQUIRE(get_text(*comment) == "C++ style.");

        comment = md_comment::parse(p, "", "//! C++ exclamation.");
        REQUIRE(get_text(*comment) == "C++ exclamation.");

        comment = md_comment::parse(p, "", "/** C style.*/");
        REQUIRE(get_text(*comment) == "C style.");

        comment = md_comment::parse(p, "", "/*! C exclamation.*/");
        REQUIRE(get_text(*comment) == "C exclamation.");

        comment = md_comment::parse(p, "", "/** C style\n * multiline.\n*/");
        REQUIRE(get_text(*comment) == "C style\nmultiline.");

        comment = md_comment::parse(p, "", "/** C style\n/// C++ multiline.*/");
        REQUIRE(get_text(*comment) == "C style\n/// C++ multiline.");
    }
    SECTION("simple parsing")
    {
        auto comment = md_comment::parse(p, "", R"(/// Hello World.)");
        auto count   = 0u;
        for (auto& child : *comment)
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
        auto comment = md_comment::parse(p, "", R"(/// \brief A
                                                 ///
                                                 /// \details B
                                                 /// C
                                               )");

        auto count = 0u;
        for (auto& child : *comment)
        {
            REQUIRE(child.get_entity_type() == md_entity::paragraph_t);
            auto& paragraph = dynamic_cast<const md_paragraph&>(child);

            if (get_text(paragraph) == "A")
            {
                ++count;
                REQUIRE(paragraph.get_section_type() == section_type::brief);
            }
            else if (get_text(paragraph) == "B\nC")
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
        auto comment = md_comment::parse(p, "", R"(///  A
                                                ///
                                                /// B
                                                /// C /// C
                                               )");

        auto count = 0u;
        for (auto& child : *comment)
        {
            REQUIRE(child.get_entity_type() == md_entity::paragraph_t);
            auto& paragraph = dynamic_cast<const md_paragraph&>(child);

            if (get_text(paragraph) == "A")
            {
                ++count;
                REQUIRE(paragraph.get_section_type() == section_type::brief);
            }
            else if (get_text(paragraph) == "B\nC /// C")
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
        auto comment = md_comment::parse(p, "", R"(/// \effects A A
                                                /// A A
                                                ///
                                                /// \returns B B
                                                ///
                                                /// \error_conditions C C)");

        auto count = 0u;
        for (auto& child : *comment)
        {
            REQUIRE(child.get_entity_type() == md_entity::paragraph_t);
            auto& paragraph = dynamic_cast<const md_paragraph&>(child);
            INFO(get_text(paragraph));

            if (get_text(paragraph) == " A A\nA A")
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
