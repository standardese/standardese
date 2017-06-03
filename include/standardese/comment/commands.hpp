// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_COMMANDS_HPP_INCLUDED
#define STANDARDESE_COMMENT_COMMANDS_HPP_INCLUDED

#include <cassert>

#include <standardese/markup/doc_section.hpp>

namespace standardese
{
    namespace comment
    {
        using markup::section_type;

        /// The documentation special commands.
        enum class command_type : unsigned
        {
            invalid = unsigned(section_type::count),

            exclude,
            unique_name,
            synopsis,

            group,
            module,
            output_section,

            entity,
            file,
            param,
            tparam,
            base,

            count,
        };

        static_assert(unsigned(section_type::invalid) == unsigned(command_type::invalid), "");

        /// \returns Whether or not the given number corresponds to a section.
        inline constexpr bool is_section(unsigned c)
        {
            return c < unsigned(section_type::count);
        }

        /// \returns The section corresponding to the given number.
        /// \requires `is_section(c)`
        inline section_type make_section(unsigned c)
        {
            assert(is_section(c));
            return section_type(c);
        }

        /// \returns Whether or not the given number corresponds to a command.
        inline constexpr bool is_command(unsigned c)
        {
            return c > unsigned(section_type::count) && c < unsigned(command_type::count);
        }

        /// \returns The command corresponding to the given number.
        /// \requires `is_command(c)`.
        inline command_type make_command(unsigned c)
        {
            assert(is_command(c));
            return command_type(c);
        }
    }
} // namespace standardese::comment

#endif // STANDARDESE_COMMENT_COMMANDS_HPP_INCLUDED
