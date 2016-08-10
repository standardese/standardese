// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_PREPROCESSOR_HPP_INCLUDED
#define STANDARDESE_CPP_PREPROCESSOR_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>

namespace standardese
{
    class cpp_inclusion_directive final : public cpp_entity
    {
    public:
        enum kind
        {
            system,
            local
        };

        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::inclusion_directive_t;
        }

        static cpp_ptr<cpp_inclusion_directive> parse(translation_unit& tu, cpp_cursor cur,
                                                      const cpp_entity& parent);

        cpp_name get_name() const override
        {
            return "inclusion directive";
        }

        kind get_kind() const STANDARDESE_NOEXCEPT
        {
            return kind_;
        }

        const std::string& get_file_name() const STANDARDESE_NOEXCEPT
        {
            return file_name_;
        }

    private:
        cpp_inclusion_directive(cpp_cursor cur, const cpp_entity& parent, std::string file_name,
                                kind k)
        : cpp_entity(get_entity_type(), cur, parent), file_name_(std::move(file_name)), kind_(k)
        {
        }

        std::string file_name_;
        kind        kind_;

        friend detail::cpp_ptr_access;
    };

    class cpp_macro_definition final : public cpp_entity
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::macro_definition_t;
        }

        static cpp_ptr<cpp_macro_definition> parse(translation_unit& tu, cpp_cursor cur,
                                                   const cpp_entity& parent);

        bool is_function_macro() const STANDARDESE_NOEXCEPT
        {
            return !args_.empty();
        }

        /// Returns the argument string including brackets.
        const std::string& get_argument_string() const STANDARDESE_NOEXCEPT
        {
            return args_;
        }

        const std::string& get_replacement() const STANDARDESE_NOEXCEPT
        {
            return replacement_;
        }

    private:
        cpp_macro_definition(cpp_cursor cur, const cpp_entity& parent, std::string args,
                             std::string replacement)
        : cpp_entity(get_entity_type(), cur, parent),
          args_(std::move(args)),
          replacement_(std::move(replacement))
        {
        }

        std::string args_, replacement_;

        friend detail::cpp_ptr_access;
    };

    namespace detail
    {
        // returns the command line definition needed for the context
        std::string get_cmd_definition(translation_unit& tu, cpp_cursor expansion_ref);
    }
} // namespace standardese

#endif // STANDARDESE_CPP_PREPROCESSOR_HPP_INCLUDED
