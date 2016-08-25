// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_ENUM_HPP_INCLUDED
#define STANDARDESE_CPP_ENUM_HPP_INCLUDED

#include <standardese/cpp_type.hpp>

namespace standardese
{
    class cpp_enum_value : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_enum_value> parse(translation_unit& tu, cpp_cursor cur,
                                             const cpp_entity& parent);

        bool is_explicitly_given() const STANDARDESE_NOEXCEPT
        {
            return explicit_;
        }

        virtual cpp_name get_scope() const override;

    protected:
        cpp_enum_value(cpp_entity::type t, cpp_cursor cur, const cpp_entity& parent,
                       bool is_explicit)
        : cpp_entity(t, cur, parent), explicit_(is_explicit)
        {
        }

    private:
        bool explicit_;

        friend detail::cpp_ptr_access;
    };

    class cpp_signed_enum_value final : public cpp_enum_value
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::signed_enum_value_t;
        }

        long long get_value() const STANDARDESE_NOEXCEPT
        {
            return value_;
        }

    private:
        cpp_signed_enum_value(cpp_cursor cur, const cpp_entity& parent, long long value,
                              bool is_explicit)
        : cpp_enum_value(get_entity_type(), cur, parent, is_explicit), value_(value)
        {
        }

        long long value_;

        friend detail::cpp_ptr_access;
    };

    class cpp_unsigned_enum_value final : public cpp_enum_value
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::unsigned_enum_value_t;
        }

        unsigned long long get_value() const STANDARDESE_NOEXCEPT
        {
            return value_;
        }

    private:
        cpp_unsigned_enum_value(cpp_cursor cur, const cpp_entity& parent, unsigned long long value,
                                bool is_explicit)
        : cpp_enum_value(get_entity_type(), cur, parent, is_explicit), value_(value)
        {
        }

        unsigned long long value_;

        friend detail::cpp_ptr_access;
    };

    class cpp_expression_enum_value final : public cpp_enum_value
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::expression_enum_value_t;
        }

        const string& get_value() const STANDARDESE_NOEXCEPT
        {
            return value_;
        }

    private:
        cpp_expression_enum_value(cpp_cursor cur, const cpp_entity& parent, string value)
        : cpp_enum_value(get_entity_type(), cur, parent, !value.empty()), value_(std::move(value))
        {
        }

        string value_;

        friend detail::cpp_ptr_access;
    };

    class cpp_enum final : public cpp_type, public cpp_entity_container<cpp_enum_value>
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::enum_t;
        }

        static cpp_ptr<cpp_enum> parse(translation_unit& tu, cpp_cursor cur,
                                       const cpp_entity& parent);

        void add_enum_value(cpp_ptr<cpp_enum_value> value)
        {
            cpp_entity_container<cpp_enum_value>::add_entity(this, std::move(value));
        }

        bool is_scoped() const STANDARDESE_NOEXCEPT
        {
            return is_scoped_;
        }

        const cpp_type_ref& get_underlying_type() const STANDARDESE_NOEXCEPT
        {
            return underlying_;
        }

    private:
        cpp_enum(cpp_cursor cur, const cpp_entity& parent, cpp_type_ref underlying, bool is_scoped)
        : cpp_type(get_entity_type(), cur, parent),
          underlying_(std::move(underlying)),
          is_scoped_(is_scoped)
        {
        }

        cpp_type_ref underlying_;
        bool         is_scoped_;

        friend detail::cpp_ptr_access;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_ENUM_HPP_INCLUDED
