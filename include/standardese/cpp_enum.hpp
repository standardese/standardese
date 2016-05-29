// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_ENUM_HPP_INCLUDED
#define STANDARDESE_CPP_ENUM_HPP_INCLUDED

#include <standardese/cpp_type.hpp>

namespace standardese
{
    class cpp_enum_value
    : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_enum_value> parse(translation_unit &tu,  cpp_name scope, cpp_cursor cur);

        cpp_enum_value(cpp_name scope, cpp_name name)
        : cpp_enum_value(enum_value_t, std::move(scope), std::move(name)) {}

        bool is_explicitly_given() const STANDARDESE_NOEXCEPT
        {
            return explicit_;
        }

    protected:
        cpp_enum_value(cpp_entity::type t, cpp_name scope,
                       cpp_name name)
        : cpp_entity(t, std::move(scope), std::move(name)),
          explicit_(false) {}

    private:
        bool explicit_;
    };

    class cpp_signed_enum_value
    : public cpp_enum_value
    {
    public:
        cpp_signed_enum_value(cpp_name scope, cpp_name name,
                              long long value)
        : cpp_enum_value(signed_enum_value_t, std::move(scope),
                         std::move(name)),
          value_(value) {}

        long long get_value() const STANDARDESE_NOEXCEPT
        {
            return value_;
        }

    private:
        long long value_;
    };

    class cpp_unsigned_enum_value
    : public cpp_enum_value
    {
    public:
        cpp_unsigned_enum_value(cpp_name scope, cpp_name name,
                                unsigned long long value)
        : cpp_enum_value(unsigned_enum_value_t, std::move(scope),
                         std::move(name)),
          value_(value) {}

        unsigned long long get_value() const STANDARDESE_NOEXCEPT
        {
            return value_;
        }

    private:
        unsigned long long value_;
    };

    class cpp_enum
    : public cpp_type, public cpp_entity_container<cpp_enum_value>
    {
    public:
        class parser
        : public cpp_entity_parser
        {
        public:
            parser(translation_unit &tu, cpp_name scope, cpp_cursor cur);

            void add_entity(cpp_entity_ptr ptr) override;

            cpp_name scope_name() override;

            cpp_entity_ptr finish(const standardese::parser &par) override;

        private:
            cpp_ptr<cpp_enum> enum_;
        };

        cpp_enum(cpp_name scope, cpp_name name,
                 CXType type, cpp_type_ref underlying)
        : cpp_type(enum_t, std::move(scope), std::move(name), type),
          underlying_(std::move(underlying)),
          is_scoped_(false) {}

        void add_enum_value(cpp_ptr<cpp_enum_value> value)
        {
            cpp_entity_container::add_entity(std::move(value));
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
        cpp_type_ref underlying_;
        bool is_scoped_;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_ENUM_HPP_INCLUDED
