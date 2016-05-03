// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment.hpp>

#include <cassert>
#include <iostream>

using namespace standardese;

namespace
{
    char command_character = '\\';

    std::string section_names[std::size_t(section_type::count)];

    void init_sections()
    {
        comment::parser::set_section_name(section_type::brief, "brief");
        comment::parser::set_section_name(section_type::detail, "detail");
    }

    auto initializer = (init_sections(), 0);

    section_type parse_type(const char *name)
    {
        auto i = 0;
        for (auto& sname : section_names)
        {
            if (sname == name)
                break;
            ++i;
        }

        return section_type(i);
    }

    // allows out of range access
    class comment_stream
    {
    public:
        comment_stream(const cpp_raw_comment &c)
        : comment_(&c) {}

        char operator[](std::size_t i) const STANDARDESE_NOEXCEPT
        {
            if (i >= size())
                return '\n'; // to terminate any section
            return (*comment_)[i];
        }

        std::size_t size() const STANDARDESE_NOEXCEPT
        {
            return comment_->size();
        }

    private:
        const cpp_raw_comment *comment_;
    };

    void trim_whitespace(std::string &str)
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

    void parse_error(const std::string &message)
    {
        std::cerr << "Comment parse error: " << message << '\n';
        std::abort();
    }
}

void comment::parser::set_command_character(char c)
{
    command_character = c;
}

void comment::parser::set_section_name(section_type t, std::string name)
{
    section_names[std::size_t(t)] = name;
}

comment::parser::parser(const cpp_raw_comment &raw_comment)
{
    comment_stream stream(raw_comment);
    auto cur_section_t = section_type::brief;
    std::string cur_body;

    auto finish_section = [&]
                {
                    trim_whitespace(cur_body);
                    if (cur_body.empty())
                        return;

                    comment_.sections_.emplace_back(cur_section_t, section_names[int(cur_section_t)], cur_body);
                    cur_section_t = section_type::detail;
                    cur_body.clear();
                };

    auto i = 0u;
    while (i < stream.size())
    {
        if (stream[i] == '/')
        {
            // ignore all comment characters
            while (stream[i] == '/')
                ++i;
        }
        else if (stream[i] == '\n')
        {
            // section is finished
            ++i;
            finish_section();
        }
        else if (stream[i] == command_character)
        {
            // add new section
            std::string section_name;
            while (stream[++i] != ' ')
                section_name += stream[i];

            auto type = parse_type(section_name.c_str());
            if (type == section_type::invalid)
                parse_error("Invalid section name " + section_name);

            finish_section();
            cur_section_t = type;
        }
        else
        {
            // add to body
            cur_body += stream[i];
            ++i;
        }
    }
    finish_section();
}

comment comment::parser::finish()
{
    return comment_;
}
