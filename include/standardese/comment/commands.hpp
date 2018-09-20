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

        verbatim,
        end,

        exclude,
        unique_name,
        output_name,
        synopsis,

        group,
        module,
        output_section,

        entity,
        file,

        count,
    };

    enum class inline_type : unsigned
    {
        invalid = unsigned(command_type::count),

        param,
        tparam,
        base,

        count,
    };

    static_assert(unsigned(section_type::count) == unsigned(command_type::invalid), "");
    static_assert(unsigned(command_type::count) == unsigned(inline_type::invalid), "");

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

    /// \returns Whether or not the given number corresponds to an inline.
    inline constexpr bool is_inline(unsigned c)
    {
        return c > unsigned(command_type::count) && c < unsigned(inline_type::count);
    }

    /// \returns The inline corresponding to the given number.
    /// \requires `is_inline(c)`.
    inline inline_type make_inline(unsigned c)
    {
        assert(is_inline(c));
        return inline_type(c);
    }
} // namespace comment
} // namespace standardese

#endif // STANDARDESE_COMMENT_COMMANDS_HPP_INCLUDED
