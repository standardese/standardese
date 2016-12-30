// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_SECTION_HPP_INCLUDED
#define STANDARDESE_SECTION_HPP_INCLUDED

#include <cassert>

#include <standardese/string.hpp>

namespace standardese
{
    enum class section_type : unsigned
    {
        brief,
        details,

        // [structure.specifications]/3 sections
        requires,
        effects,
        synchronization,
        postconditions,
        returns,
        throws,
        complexity,
        remarks,
        error_conditions,
        notes,

        count,
        invalid = count
    };

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

    inline bool is_section(unsigned c) STANDARDESE_NOEXCEPT
    {
        return c < unsigned(section_type::count);
    }

    inline section_type make_section(unsigned c) STANDARDESE_NOEXCEPT
    {
        assert(is_section(c));
        return section_type(c);
    }

    inline bool is_command(unsigned c) STANDARDESE_NOEXCEPT
    {
        return c > unsigned(section_type::count) && c < unsigned(command_type::count);
    }

    inline command_type make_command(unsigned c) STANDARDESE_NOEXCEPT
    {
        assert(is_command(c));
        return command_type(c);
    }
} // namespace standardese

#endif // STANDARDESE_SECTION_HPP_INCLUDED
