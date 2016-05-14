// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_PREPROCESSOR_HPP_INCLUDED
#define STANDARDESE_CPP_PREPROCESSOR_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>
#include "cpp_cursor.hpp"

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

        static cpp_ptr<cpp_inclusion_directive> parse(translation_unit &tu,  cpp_cursor cur);

        cpp_inclusion_directive(cpp_name file_name, cpp_raw_comment comment, kind k)
        : cpp_entity(inclusion_directive_t, "", std::move(file_name), std::move(comment)),
          kind_(k) {}

        kind get_kind() const STANDARDESE_NOEXCEPT
        {
            return kind_;
        }

    private:
        kind kind_;
    };

    class cpp_macro_definition
    : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_macro_definition> parse(translation_unit &tu,  cpp_cursor cur);

        cpp_macro_definition(cpp_name name, cpp_raw_comment c,
                             std::string args, std::string rep)
        : cpp_entity(macro_definition_t, "", std::move(name), std::move(c)),
          args_(std::move(args)), replacement_(std::move(rep)) {}

        bool is_function_macro() const STANDARDESE_NOEXCEPT
        {
            return !args_.empty();
        }

        /// Returns the argument string including brackets.
        const std::string &get_argument_string() const STANDARDESE_NOEXCEPT
        {
            return args_;
        }

        const std::string &get_replacement() const STANDARDESE_NOEXCEPT
        {
            return replacement_;
        }

    private:
        std::string args_, replacement_;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_PREPROCESSOR_HPP_INCLUDED
