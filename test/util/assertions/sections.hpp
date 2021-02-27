// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_TEST_UTIL_ASSERTIONS_SECTIONS_HPP_INCLUDED
#define STANDARDESE_TEST_UTIL_ASSERTIONS_SECTIONS_HPP_INCLUDED

#include <string>

#include "../../../include/standardese/comment/doc_comment.hpp"
#include "../../../include/standardese/comment/parser.hpp"

namespace standardese::test::util::assertions {

void CHECK_BRIEF_EQUIVALENT_TO(const standardese::comment::doc_comment& entity, std::string xml);
void CHECK_BRIEF_EQUIVALENT_TO(const standardese::comment::unmatched_doc_comment& imline, std::string xml);
void CHECK_BRIEF_EQUIVALENT_TO(const standardese::comment::parse_result& result, std::string xml);

void CHECK_SECTIONS_EQUIVALENT_TO(const comment::doc_comment& entity, std::vector<std::string> xml);
void CHECK_SECTIONS_EQUIVALENT_TO(const comment::doc_comment& entity, std::string xml);
void CHECK_SECTIONS_EQUIVALENT_TO(const comment::unmatched_doc_comment& imline, std::vector<std::string> xml);
void CHECK_SECTIONS_EQUIVALENT_TO(const comment::unmatched_doc_comment& imline, std::string xml);
void CHECK_SECTIONS_EQUIVALENT_TO(const comment::parse_result& result, std::vector<std::string> xml);
void CHECK_SECTIONS_EQUIVALENT_TO(const comment::parse_result& result, std::string xml);

void CHECK_INLINES_EQUIVALENT_TO(const comment::parse_result& result, std::vector<std::vector<std::string>> xml);
void CHECK_INLINES_EQUIVALENT_TO(const comment::parse_result& result, std::vector<std::string> xml);
void CHECK_INLINES_EQUIVALENT_TO(const comment::parse_result& result, std::string xml);

}

#endif
