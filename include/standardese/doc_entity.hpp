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
        doc_entity(const parser& p, const cpp_entity& e)
        : comment_(comment::parse(p, e.get_name(), e.get_comment())), entity_(&e)
        {
        }

        doc_entity(comment com) : comment_(std::move(com))
        {
        }

        const comment& get_comment() const STANDARDESE_NOEXCEPT
        {
            return comment_;
        }

        bool has_cpp_entity() const STANDARDESE_NOEXCEPT
        {
            return entity_ != nullptr;
        }

        const cpp_entity& get_cpp_entity() const STANDARDESE_NOEXCEPT
        {
            return *entity_;
        }

    private:
        comment           comment_;
        const cpp_entity* entity_;
    };
} // namespace standardese

#endif // STANDARDESE_DOC_ENTITY_HPP_INCLUDED
