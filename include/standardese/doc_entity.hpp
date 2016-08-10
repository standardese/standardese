// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DOC_ENTITY_HPP_INCLUDED
#define STANDARDESE_DOC_ENTITY_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>

namespace standardese
{
    class comment;

    class doc_entity
    {
    public:
        doc_entity(const parser& p, const cpp_entity& entity, cpp_name output_name);

        bool has_comment() const STANDARDESE_NOEXCEPT
        {
            return comment_ != nullptr;
        }

        const comment& get_comment() const STANDARDESE_NOEXCEPT
        {
            return *comment_;
        }

        const cpp_entity& get_cpp_entity() const STANDARDESE_NOEXCEPT
        {
            return *entity_;
        }

        cpp_entity::type get_entity_type() const;

        cpp_name get_name() const
        {
            return get_cpp_entity().get_name();
        }

        cpp_name get_full_name() const
        {
            return get_cpp_entity().get_full_name();
        }

        cpp_name get_unique_name() const;

        cpp_name get_output_name() const
        {
            return output_name_;
        }

    private:
        cpp_name          output_name_;
        const cpp_entity* entity_;
        const comment*    comment_;
    };
} // namespace standardese

#endif // STANDARDESE_DOC_ENTITY_HPP_INCLUDED
