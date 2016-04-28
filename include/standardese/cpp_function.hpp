// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_FUNCTION_HPP_INCLUDED
#define STANDARDESE_CPP_FUNCTION_HPP_INCLUDED

#include <string>

#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_type.hpp>

namespace standardese
{
    class cpp_function_parameter
    : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_function_parameter> parse(cpp_cursor cur);

        cpp_function_parameter(cpp_name name, cpp_comment comment,
                               cpp_type_ref type, std::string default_value = "")
        : cpp_entity("", std::move(name), std::move(comment)),
          type_(std::move(type)), default_(std::move(default_value)) {}

        const cpp_type_ref& get_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

        const std::string& get_default_value() const STANDARDESE_NOEXCEPT
        {
            return default_;
        }

    private:
        cpp_type_ref type_;
        std::string  default_;
    };

    enum cpp_function_flags
    {
        cpp_variadic_fnc    = 1,
        cpp_constexpr_fnc   = 2,
    };

    enum cpp_function_definition
    {
        cpp_function_definition_normal,
        cpp_function_definition_deleted,
        cpp_function_definition_defaulted,
    };

    // common stuff for all functions
    class cpp_function_base
    : public cpp_entity, public cpp_entity_container<cpp_function_parameter>
    {
    public:
        void add_parameter(cpp_ptr<cpp_function_parameter> param)
        {
            cpp_entity_container::add_entity(std::move(param));
        }

        bool is_variadic() const STANDARDESE_NOEXCEPT
        {
            return flags_ & cpp_variadic_fnc;
        }

        bool is_constexpr() const STANDARDESE_NOEXCEPT
        {
            return flags_ & cpp_constexpr_fnc;
        }

        cpp_function_definition get_definition() const STANDARDESE_NOEXCEPT
        {
            return definition_;
        }

        // the part inside a noexcept(...)
        // noexcept without condition leads to "true"
        // no noexcept at all leads to "false"
        const std::string& get_noexcept() const STANDARDESE_NOEXCEPT
        {
            return noexcept_expr_;
        }

    protected:
        cpp_function_base(cpp_name scope, cpp_name name, cpp_comment comment,
                          std::string noexcept_expr,
                          int flags, cpp_function_definition def)
        : cpp_entity(std::move(scope), std::move(name), std::move(comment)),
          noexcept_expr_(std::move(noexcept_expr)),
          flags_(cpp_function_flags(flags)), definition_(def) {}

    private:
        std::string noexcept_expr_;
        cpp_function_flags flags_;
        cpp_function_definition definition_;
    };

    class cpp_function
    : public cpp_function_base
    {
    public:
        static cpp_ptr<cpp_function> parse(cpp_name scope, cpp_cursor cur);

        cpp_function(cpp_name scope, cpp_name name, cpp_comment comment,
                     cpp_type_ref return_type, std::string noexcept_expr,
                     cpp_function_flags flags,
                     cpp_function_definition def = cpp_function_definition_normal)
        : cpp_function_base(std::move(scope), std::move(name), std::move(comment),
                            std::move(noexcept_expr), flags, def),
          return_(std::move(return_type)) {}

        const cpp_type_ref& get_return_type() const STANDARDESE_NOEXCEPT
        {
            return return_;
        }

    private:
        cpp_type_ref return_;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_FUNCTION_HPP_INCLUDED
