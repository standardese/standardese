// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_PREPROCESSOR_HPP_INCLUDED
#define STANDARDESE_CPP_PREPROCESSOR_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>

namespace standardese
{
    class cpp_cursor;

    class cpp_inclusion_directive
    : public cpp_entity
    {
    public:
        enum kind
        {
            system,
            local
        };

        static cpp_ptr<cpp_inclusion_directive> parse(cpp_cursor cur);

        cpp_inclusion_directive(cpp_name file_name, cpp_comment comment, kind k)
        : cpp_entity("", std::move(file_name), std::move(comment)),
          kind_(k) {}

        kind get_kind() const STANDARDESE_NOEXCEPT
        {
            return kind_;
        }

    private:
        kind kind_;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_PREPROCESSOR_HPP_INCLUDED
