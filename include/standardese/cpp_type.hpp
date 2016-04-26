// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_TYPE_HPP_INCLUDED
#define STANDARDESE_CPP_TYPE_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>

namespace standardese
{
    class parser;
    struct cpp_cursor;

    class cpp_type
    : public cpp_entity
    {
    protected:
        cpp_type(cpp_name scope, cpp_name name, cpp_comment comment)
        : cpp_entity(std::move(scope), std::move(name), std::move(comment)) {}
    };

    class cpp_type_alias
    : public cpp_type
    {
    public:
        static cpp_ptr<cpp_type_alias> parse(const parser &p, const cpp_name &scope, cpp_cursor cur);

        cpp_type_alias(cpp_name scope, cpp_name name, cpp_comment comment,
                       cpp_name target)
        : cpp_type(std::move(scope), std::move(name), std::move(comment)),
          target_(target), unique_(std::move(target)) {}

        /// Returns the target as given in the code but without spaces.
        const cpp_name& get_target() const STANDARDESE_NOEXCEPT
        {
            return target_;
        }

        /// Returns the full target with all namespaces as libclang returns it.
        cpp_name get_full_target() const
        {
            return unique_;
        }

    private:
        cpp_type_alias(cpp_name scope, cpp_name name, cpp_comment comment)
        : cpp_type(std::move(scope), std::move(name), std::move(comment)) {}

        cpp_name target_, unique_;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_TYPE_HPP_INCLUDED
