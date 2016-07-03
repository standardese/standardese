// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_HPP_INCLUDED
#define STANDARDESE_COMMENT_HPP_INCLUDED

#include <string>
#include <vector>

#include <standardese/cpp_entity.hpp>
#include <standardese/md_entity.hpp>
#include <standardese/section.hpp>

namespace standardese
{
    class parser;

    class md_comment final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::comment_t;
        }

        static md_ptr<md_comment> parse(const parser& p, const cpp_name& name,
                                        const cpp_raw_comment& comment);

    private:
        md_comment();

        friend detail::md_ptr_access;
    };
} // namespace standardese

#endif // STANDARDESE_COMMENT_HPP_INCLUDED
