// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment.hpp>

#include <cassert>

#include <standardese/detail/sequence_stream.hpp>
#include <standardese/config.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

namespace
{
    void trim_whitespace(std::string& str)
    {
        auto start = 0u;
        while (std::isspace(str[start]))
            ++start;

        auto end = str.size();
        while (std::isspace(str[end - 1]))
            --end;

        if (end <= str.size())
            str.erase(end);
        str.erase(0, start);
    }
}

comment comment::parse(const parser& p, const cpp_name& name, const cpp_raw_comment& raw_comment)
{
    comment result;

    detail::sequence_stream<const char*> stream(raw_comment, '\n');
    auto                                 cur_section_t = section_type::brief;
    std::string                          cur_body;

    auto finish_section = [&] {
        if (cur_body.empty())
            return;
        trim_whitespace(cur_body);

        result.sections_.emplace_back(cur_section_t, cur_body);

        // when current section is brief, change to details
        // otherwise stay
        if (cur_section_t == section_type::brief)
            cur_section_t = section_type::details;

        cur_body.clear();
    };

    auto line = 0u;
    while (!stream.done())
    {
        if (line == 0u || stream.peek() == '\n')
        {
            stream.bump(); // consume newl
            ++line;

            // section is finished
            finish_section();

            // ignore all comment characters
            while (stream.peek() == ' ' || stream.peek() == '/')
                stream.bump();
        }
        else if (stream.peek() == p.get_comment_config().get_command_character())
        {
            stream.bump();

            // add new section
            std::string section_name;
            while (!std::isspace(stream.peek()))
                section_name += stream.get();

            auto type = p.get_comment_config().try_get_section(section_name);
            if (type == section_type::invalid)
                p.get_logger()->error("in comment of {}:{}: Invalid section name '{}'",
                                      name.c_str(), line, section_name);
            else
            {
                finish_section();
                cur_section_t = type;
            }
        }
        else
        {
            // add to body
            cur_body += stream.get();
        }
    }
    finish_section();

    return result;
}
