// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DOC_ENTITY_HPP_INCLUDED
#define STANDARDESE_DOC_ENTITY_HPP_INCLUDED

#include <standardese/comment.hpp>
#include <standardese/cpp_entity.hpp>

namespace standardese
{
    class doc_entity
    {
    public:
        doc_entity(const cpp_entity& entity) STANDARDESE_NOEXCEPT : entity_(&entity)
        {
        }

        bool has_comment() const STANDARDESE_NOEXCEPT
        {
            return entity_->has_comment();
        }

        const md_comment& get_comment() const STANDARDESE_NOEXCEPT
        {
            return entity_->get_comment();
        }

        const cpp_entity& get_cpp_entity() const STANDARDESE_NOEXCEPT
        {
            return *entity_;
        }

    private:
        const cpp_entity* entity_;
    };
} // namespace standardese

#endif // STANDARDESE_DOC_ENTITY_HPP_INCLUDED
