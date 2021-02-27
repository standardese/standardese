// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "../../external/catch/single_include/catch2/catch.hpp"

#include "sections.hpp"
#include "../indent.hpp"

#include "../../include/standardese/markup/generator.hpp"

namespace standardese::test::util::assertions {

using namespace standardese::comment;

void CHECK_BRIEF_EQUIVALENT_TO(const doc_comment& entity, std::string xml) {
    INFO("There must be a brief.");
    REQUIRE(entity.brief_section().has_value());

    INFO("The brief must have the expected structure.");
    CHECK(standardese::markup::as_xml(entity.brief_section().value()) == unindent(xml));
}

void CHECK_BRIEF_EQUIVALENT_TO(const unmatched_doc_comment& imline, std::string xml) {
    CHECK_BRIEF_EQUIVALENT_TO(imline.comment, xml);
}

void CHECK_BRIEF_EQUIVALENT_TO(const parse_result& result, std::string xml) {
    INFO("The parsed comment must be non-empty.");
    REQUIRE(result.comment.has_value());

    CHECK_BRIEF_EQUIVALENT_TO(result.comment.value(), xml);
}

void CHECK_SECTIONS_EQUIVALENT_TO(const doc_comment& entity, std::vector<std::string> xml) {
    INFO("There must be the expected number of sections.");
    REQUIRE(entity.sections().size() == xml.size());

    INFO("Each section must have the expected structure.");
    auto section = entity.sections().begin();
    auto expected = xml.begin();
    while (section != entity.sections().end()) {
        CHECK(markup::as_xml(*section) == unindent(*expected));
        section++;
        expected++;
    }
}

void CHECK_SECTIONS_EQUIVALENT_TO(const doc_comment& entity, std::string xml) {
    CHECK_SECTIONS_EQUIVALENT_TO(entity, std::vector{xml});
}

void CHECK_SECTIONS_EQUIVALENT_TO(const unmatched_doc_comment& imline, std::vector<std::string> xml) {
    CHECK_SECTIONS_EQUIVALENT_TO(imline.comment, xml);
}

void CHECK_SECTIONS_EQUIVALENT_TO(const unmatched_doc_comment& imline, std::string xml) {
    CHECK_SECTIONS_EQUIVALENT_TO(imline.comment, xml);
}

void CHECK_SECTIONS_EQUIVALENT_TO(const parse_result& result, std::vector<std::string> xml) {
    INFO("The comment must be non-empty.");
    REQUIRE(result.comment.has_value());

    CHECK_SECTIONS_EQUIVALENT_TO(result.comment.value(), xml);
}

void CHECK_SECTIONS_EQUIVALENT_TO(const parse_result& result, std::string xml) {
    CHECK_SECTIONS_EQUIVALENT_TO(result, std::vector(1, xml));
}

void CHECK_INLINES_EQUIVALENT_TO(const parse_result& result, std::vector<std::vector<std::string>> xml) {
    INFO("There must be the expected number of inlines.");
    REQUIRE(result.inlines.size() == xml.size());

    INFO("Each inline must have the expected structure");
    auto innline = result.inlines.begin();
    auto expected = xml.begin();
    while (innline != result.inlines.end()) {
      CHECK_SECTIONS_EQUIVALENT_TO(innline->comment, *expected);
      expected++;
      innline++;
    }
}

void CHECK_INLINES_EQUIVALENT_TO(const parse_result& result, std::vector<std::string> xml) {
    CHECK_INLINES_EQUIVALENT_TO(result, std::vector(1, xml));
}

void CHECK_INLINES_EQUIVALENT_TO(const parse_result& result, std::string xml) {
    CHECK_INLINES_EQUIVALENT_TO(result, std::vector(1, xml));
}

}
