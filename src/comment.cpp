// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment.hpp>

#include <cassert>
#include <iostream>
#include <unordered_map>

using namespace standardese;

namespace
{
    char command_character = '\\';

    std::unordered_map<std::string, section_type> section_commands;
    std::string section_names[std::size_t(section_type::count)];

    void init_sections()
    {
        #define STANDARDESE_DETAIL_SET(type, name) \
            comment::parser::set_section_name(section_type::type, name); \
            section_commands[#type] = section_type::type;

        STANDARDESE_DETAIL_SET(brief, "")
        STANDARDESE_DETAIL_SET(details, "")

        STANDARDESE_DETAIL_SET(requires, "Requires")
        STANDARDESE_DETAIL_SET(effects, "Effects")
        STANDARDESE_DETAIL_SET(synchronization, "Synchronization")
        STANDARDESE_DETAIL_SET(postconditions, "Postconditions")
        STANDARDESE_DETAIL_SET(returns, "Returns")
        STANDARDESE_DETAIL_SET(throws, "Throws")
        STANDARDESE_DETAIL_SET(complexity, "Complexity")
        STANDARDESE_DETAIL_SET(remarks, "Remarks")
        STANDARDESE_DETAIL_SET(error_conditions, "Error conditions")
        STANDARDESE_DETAIL_SET(notes, "Notes")

        #undef STANDARDESE_DETAIL_SET
    }

    auto initializer = (init_sections(), 0);

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

void comment::parser::set_section_command(section_type t, std::string name)
{
    section_commands[name] = t;
}

void comment::parser::set_section_command(const std::string &type, std::string name)
{
    auto iter = section_commands.find(type);
    if (iter == section_commands.end())
        throw std::invalid_argument("invalid section command name '" + type + "'");

    auto t = iter->second;
    section_commands.erase(iter);

    auto res = section_commands.emplace(name, t);
    if (!res.second)
        throw std::invalid_argument("section command name '" + name + "' already in use");
}

void comment::parser::set_section_name(section_type t, std::string name)
{
    assert(t != section_type::invalid);
    section_names[std::size_t(t)] = name;
}

void comment::parser::set_section_name(const std::string &type, std::string name)
{
    auto iter = section_commands.find(type);
    if (iter == section_commands.end())
        throw std::invalid_argument("invalid section command name '" + type + "'");
    section_names[int(iter->second)] = std::move(name);
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

                    // when current section is brief, change to details
                    // otherwise stay
                    if (cur_section_t == section_type::brief)
                        cur_section_t = section_type::details;

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

            auto type = section_commands[section_name];
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
