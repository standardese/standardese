// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_SYNOPSIS_HPP_INCLUDED
#define STANDARDESE_SYNOPSIS_HPP_INCLUDED

#include <unordered_set>

#include <standardese/cpp_entity.hpp>
#include <standardese/output.hpp>

namespace standardese
{
    struct entity_blacklist
    {
        std::unordered_set<cpp_name> names;
        std::unordered_set<cpp_entity::type> types;

        entity_blacklist()
        : types({cpp_entity::inclusion_directive_t,
                 cpp_entity::base_class_t,
                 cpp_entity::using_declaration_t,
                 cpp_entity::using_directive_t}) {}

        bool is_blacklisted(const cpp_entity &e) const;
    };

    void write_synopsis(output_base &out, const cpp_entity &e, const entity_blacklist &blacklist);
} // namespace standardese

#endif // STANDARDESE_SYNOPSIS_HPP_INCLUDED
