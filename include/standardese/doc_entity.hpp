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
        doc_entity(const cpp_entity& entity) STANDARDESE_NOEXCEPT
            : entity_(&entity),
              comment_(entity.has_comment() ? &entity.get_comment() : nullptr)
        {
        }

        doc_entity(const cpp_entity& entity, const md_comment& comment) STANDARDESE_NOEXCEPT
            : entity_(&entity),
              comment_(&comment)
        {
        }

        bool has_comment() const STANDARDESE_NOEXCEPT
        {
            return comment_ != nullptr;
        }

        const md_comment& get_comment() const STANDARDESE_NOEXCEPT
        {
            return *comment_;
        }

        const cpp_entity& get_cpp_entity() const STANDARDESE_NOEXCEPT
        {
            return *entity_;
        }

        cpp_name get_unique_name() const STANDARDESE_NOEXCEPT
        {
            if (has_comment() && get_comment().has_unique_name())
                return get_comment().get_unique_name();
            return entity_->get_unique_name();
        }

        cpp_name get_output_name() const STANDARDESE_NOEXCEPT
        {
            assert(has_comment());
            return get_comment().get_output_name();
        }

    private:
        const cpp_entity* entity_;
        const md_comment* comment_;
    };
} // namespace standardese

#endif // STANDARDESE_DOC_ENTITY_HPP_INCLUDED
