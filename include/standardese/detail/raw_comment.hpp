// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIl_RAW_COMMENT_HPP_INCLUDED
#define STANDARDESE_DETAIl_RAW_COMMENT_HPP_INCLUDED

#include <string>
#include <vector>

namespace standardese
{
    namespace detail
    {
        struct raw_comment
        {
            std::string content;
            unsigned    count_lines, end_line;

            raw_comment(std::string content, unsigned count_lines, unsigned end_line)
            : content(std::move(content)), count_lines(count_lines), end_line(end_line)
            {
            }
        };

        bool keep_comment(const char* comment);

        std::vector<raw_comment> read_comments(const std::string& source);
    } // namespace detail
} // namespace standardese

#endif // STANDARDESE_DETAIL_RAW_COMMENT_HPP_INCLUDED
