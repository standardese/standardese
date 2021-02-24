// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
//               2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_COMMANDS_HPP_INCLUDED
#define STANDARDESE_COMMENT_COMMANDS_HPP_INCLUDED

namespace standardese
{
namespace comment
{
    /// The documentation special commands.
    enum class command_type
    {
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

    /// The type of a documentation section.
    enum class section_type
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

        preconditions, //< For consistency with postconditions.

        // proposed by p0788, not including ensures and expects
        constraints, //< Compile-time requirements.
        diagnostics, //< Compile-time requirements that will yield error message
                     //(`static_assert()`).

        see,

        count,
    };

    enum class inline_type
    {
        param,
        tparam,
        base,

        count,
    };

} // namespace comment
} // namespace standardese

#endif // STANDARDESE_COMMENT_COMMANDS_HPP_INCLUDED
