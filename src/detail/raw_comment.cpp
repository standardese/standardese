// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/raw_comment.hpp>

#include <cassert>
#include <cstring>

using namespace standardese;

namespace
{
    bool is_whitespace(char c)
    {
        return c == ' ' || c == '\t';
    }

    enum class comment_style
    {
        none,
        cpp,
        c,
        end_of_line,
    };

    comment_style get_comment_style(const char*& ptr)
    {
        if (std::strncmp(ptr, "///", 3) == 0 || std::strncmp(ptr, "//!", 3) == 0)
        {
            ptr += 3;
            return comment_style::cpp;
        }
        else if (std::strncmp(ptr, "/**", 3) == 0 || std::strncmp(ptr, "/*!", 3) == 0)
        {
            ptr += 3;
            return comment_style::c;
        }
        else if (std::strncmp(ptr, "//<", 3) == 0)
        {
            ptr += 3;
            return comment_style::end_of_line;
        }

        return comment_style::none;
    }

    detail::raw_comment parse_cpp_comment(const char*& ptr, unsigned& cur_line)
    {
        // only skip one whitespace
        if (is_whitespace(*ptr))
            ++ptr;

        std::string content;
        for (; *ptr != '\n'; ++ptr)
            content += *ptr;

        assert(*ptr == '\n');
        ++cur_line;
        return {std::move(content), 1, cur_line - 1};
    }

    void skip_c_doc_comment_continuation(const char*& ptr)
    {
        assert(ptr[-1] == '\n');
        while (is_whitespace(*ptr))
            ++ptr;

        if (*ptr == '*' && ptr[1] != '/')
        {
            ++ptr;
            // only skip next whitespace
            if (is_whitespace(*ptr))
                ++ptr;
        }
    }

    bool is_c_doc_comment_end(const char*& ptr)
    {
        if (std::strncmp(ptr, "**/", 3) == 0)
        {
            ptr += 3;
            return true;
        }
        else if (std::strncmp(ptr, "*/", 2) == 0)
        {
            ptr += 2;
            return true;
        }

        return false;
    }

    detail::raw_comment parse_c_comment(const char*& ptr, unsigned& cur_line)
    {
        while (is_whitespace(*ptr))
            ++ptr;

        std::string content;
        auto        lines         = 1u;
        auto        needs_newline = false;
        while (true)
        {
            if (*ptr == '\n')
            {
                while (!content.empty() && is_whitespace(content.back()))
                    content.pop_back();
                needs_newline = true;

                ++ptr;
                ++cur_line;
                ++lines;

                skip_c_doc_comment_continuation(ptr);
                // handle empty line
                if (*ptr == '\n')
                    // need to append '\n', otherwise will be skipped
                    content += '\n';
            }
            else if (is_c_doc_comment_end(ptr))
                break;
            else
            {
                if (needs_newline)
                {
                    content += '\n';
                    needs_newline = false;
                }
                content += *ptr++;
            }
        }
        assert(ptr[-1] == '/');
        --ptr;

        assert(!content.empty() && content.back() != '\n');
        while (is_whitespace(content.back()))
            content.pop_back();

        return {std::move(content), lines, cur_line};
    }

    bool can_merge(comment_style a, comment_style b)
    {
        if (a == comment_style::end_of_line)
            // end of line can only merge with a following cpp
            return b == comment_style::cpp;
        else if (a == comment_style::cpp)
            // C++ style can only merge with other C++ styles
            return b == comment_style::cpp;
        // otherwise cannot merge
        return false;
    }

    std::vector<detail::raw_comment> normalize(
        const std::vector<std::pair<detail::raw_comment, comment_style>>& comments)
    {
        std::vector<detail::raw_comment> results;

        for (auto iter = comments.begin(); iter != comments.end(); ++iter)
        {
            auto cur_content = iter->first.content;
            auto cur_count   = iter->first.count_lines;
            auto cur_end     = iter->first.end_line;
            while (std::next(iter) != comments.end()
                   && can_merge(iter->second, std::next(iter)->second)
                   && std::next(iter)->first.end_line
                          == cur_end + std::next(iter)->first.count_lines)
            {
                ++iter;
                cur_end = iter->first.end_line;
                cur_count += iter->first.count_lines;
                cur_content += '\n';
                cur_content += iter->first.content;
            }

            results.emplace_back(std::move(cur_content), cur_count, cur_end);
        }

        return results;
    }
}

std::vector<detail::raw_comment> detail::read_comments(const std::string& source)
{
    assert(source.back() == '\n');
    std::vector<std::pair<detail::raw_comment, comment_style>> comments;

    auto cur_line = 1u;
    for (auto ptr = source.c_str(); *ptr; ++ptr)
    {
        auto style = get_comment_style(ptr);
        if (style != comment_style::none)
        {
            if (style == comment_style::c)
                comments.emplace_back(parse_c_comment(ptr, cur_line), style);
            else
                comments.emplace_back(parse_cpp_comment(ptr, cur_line), style);
        }
        else if (*ptr == '\n')
            ++cur_line;
    }

    return normalize(comments);
}
