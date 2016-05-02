// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_TEMPLATE_HPP_INCLUDED
#define STANDARDESE_CPP_TEMPLATE_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_type.hpp>

namespace standardese
{
    class cpp_template_ref
    {
    public:
        cpp_template_ref()
        : cpp_template_ref(cpp_cursor({}), "") {}

        cpp_template_ref(cpp_cursor ref, cpp_name given)
        : given_(std::move(given)), declaration_(clang_getCursorReferenced(ref)) {}

        /// Returns the name as specified in the source.
        const cpp_name& get_name() const STANDARDESE_NOEXCEPT
        {
            return given_;
        }

        /// Returns the full name with all scopes.
        cpp_name get_full_name() const;

        /// Returns a cursor to the declaration of the template.
        cpp_cursor get_declaration() const STANDARDESE_NOEXCEPT
        {
            return declaration_;
        }

    private:
        cpp_name given_;
        cpp_cursor declaration_;
    };

    class cpp_template_parameter
    : public cpp_parameter_base
    {
    public:
        static cpp_ptr<cpp_template_parameter> try_parse(cpp_cursor cur);

        bool is_variadic() const STANDARDESE_NOEXCEPT
        {
            return variadic_;
        }

    protected:
        cpp_template_parameter(cpp_name name, cpp_comment comment,
                               bool is_variadic)
        : cpp_parameter_base(std::move(name), std::move(comment)),
          variadic_(is_variadic) {}

    private:
        bool variadic_;
    };

    class cpp_template_type_parameter
    : public cpp_template_parameter
    {
    public:
        static cpp_ptr<cpp_template_type_parameter> parse(cpp_cursor cur);

        cpp_template_type_parameter(cpp_name name, cpp_comment comment,
                                    cpp_type_ref def, bool is_variadic)
        : cpp_template_parameter(std::move(name), std::move(comment), is_variadic),
          default_(std::move(def)) {}

        bool has_default_type() const STANDARDESE_NOEXCEPT
        {
            return !default_.get_name().empty();
        }

        const cpp_type_ref& get_default_type() const STANDARDESE_NOEXCEPT
        {
            return default_;
        }

    private:
        cpp_type_ref default_;
    };

    class cpp_non_type_template_parameter
    : public cpp_template_parameter
    {
    public:
        static cpp_ptr<cpp_non_type_template_parameter> parse(cpp_cursor cur);

        cpp_non_type_template_parameter(cpp_name name, cpp_comment comment,
                                        cpp_type_ref type, std::string default_value,
                                        bool is_variadic)
        : cpp_template_parameter(std::move(name), std::move(comment), is_variadic),
          type_(std::move(type)), default_(std::move(default_value)) {}

        const cpp_type_ref& get_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

        bool has_default_value() const STANDARDESE_NOEXCEPT
        {
            return !default_.empty();
        }

        const std::string& get_default_value() const STANDARDESE_NOEXCEPT
        {
            return default_;
        }

    private:
        cpp_type_ref type_;
        std::string default_;
    };

    class cpp_template_template_parameter
    : public cpp_template_parameter, public cpp_entity_container<cpp_template_parameter>
    {
    public:
        static cpp_ptr<cpp_template_template_parameter> parse(cpp_cursor cur);

        cpp_template_template_parameter(cpp_name name, cpp_comment comment,
                                        cpp_template_ref def, bool is_variadic)
        : cpp_template_parameter(std::move(name), std::move(comment), is_variadic),
          default_(std::move(def)) {}

        void add_paramter(cpp_ptr<cpp_template_parameter> param)
        {
            cpp_entity_container::add_entity(std::move(param));
        }

        bool has_default_template() const STANDARDESE_NOEXCEPT
        {
            return !default_.get_name().empty();
        }

        const cpp_template_ref& get_default_template() const STANDARDESE_NOEXCEPT
        {
            return default_;
        }

    private:
        cpp_template_ref default_;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_TEMPLATE_HPP_INCLUDED
