// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_SECTION_HPP_INCLUDED
#define STANDARDESE_SECTION_HPP_INCLUDED

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

    struct section
    {
        string       body;
        section_type type;

        section(section_type t, string body) : body(std::move(body)), type(t)
        {
        }
    };
} // namespace standardese

#endif // STANDARDESE_SECTION_HPP_INCLUDED
