// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_HPP_INCLUDED
#define STANDARDESE_COMMENT_HPP_INCLUDED

#include <string>
#include <vector>

#include <standardese/cpp_entity.hpp>
#include <standardese/section.hpp>

namespace standardese
{
    class parser;

    class comment
    {
    public:
        static comment parse(const parser& p, const cpp_name& name, const cpp_raw_comment& comment);

        const std::vector<section>& get_sections() const STANDARDESE_NOEXCEPT
        {
            return sections_;
        }

    private:
        std::vector<section> sections_;
    };
} // namespace standardese

#endif // STANDARDESE_COMMENT_HPP_INCLUDED
