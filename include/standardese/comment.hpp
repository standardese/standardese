// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_HPP_INCLUDED
#define STANDARDESE_COMMENT_HPP_INCLUDED

#include <string>
#include <vector>

#include <standardese/cpp_entity.hpp>

namespace standardese
{
    enum class section_type
    {
        brief,
        detail,

        count,
        invalid = count
    };

    struct section
    {
        std::string name, body;
        section_type type;

        section(section_type t, std::string name, std::string body)
        : name(std::move(name)), body(std::move(body)), type(t) {}
    };

    class comment
    {
    public:
        class parser;

        const std::vector<section>& get_sections() const STANDARDESE_NOEXCEPT
        {
            return sections_;
        }

    private:
        std::vector<section> sections_;

        friend parser;
    };

    class comment::parser
    {
    public:
        /// Sets the character used to introduce a special command.
        /// Default is "\".
        static void set_command_character(char c);

        /// Sets the name for a section.
        static void set_section_name(section_type t, std::string name);

        parser(const cpp_raw_comment &raw_comment);

        comment finish();

    private:
        comment comment_;
    };
} // namespace standardese

#endif // STANDARDESE_COMMENT_HPP_INCLUDED
