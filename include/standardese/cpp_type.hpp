// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_TYPE_HPP_INCLUDED
#define STANDARDESE_CPP_TYPE_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>
#include "parser.hpp"

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

    class cpp_enum_value
    : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_enum_value> parse(cpp_name scope, cpp_cursor cur);

        cpp_enum_value(cpp_name scope, cpp_name name, cpp_comment comment)
        : cpp_entity(std::move(scope), std::move(name), std::move(comment)),
          explicit_(false) {}

        bool is_explicitly_given() const STANDARDESE_NOEXCEPT
        {
            return explicit_;
        }

    private:
        bool explicit_;
    };

    class cpp_signed_enum_value
    : public cpp_enum_value
    {
    public:
        cpp_signed_enum_value(cpp_name scope, cpp_name name, cpp_comment comment,
                              long long value)
        : cpp_enum_value(std::move(scope), std::move(name), std::move(comment)),
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
        cpp_unsigned_enum_value(cpp_name scope, cpp_name name, cpp_comment comment,
                                unsigned long long value)
        : cpp_enum_value(std::move(scope), std::move(name), std::move(comment)),
          value_(value) {}

        unsigned long long get_value() const STANDARDESE_NOEXCEPT
        {
            return value_;
        }

    private:
        unsigned long long value_;
    };

    class cpp_enum
    : public cpp_type, public cpp_entity_container
    {
    public:
        class parser
        : public cpp_entity_parser
        {
        public:
            parser(cpp_name scope, cpp_cursor cur);

            void add_entity(cpp_entity_ptr ptr) override
            {
                enum_->add_entity(std::move(ptr));
            }

            cpp_name scope_name() override
            {
                return enum_->is_scoped_ ? enum_->get_name() : "";
            }

            cpp_entity_ptr finish(const standardese::parser &par) override;

        private:
            cpp_ptr<cpp_enum> enum_;
        };

        bool is_scoped() const STANDARDESE_NOEXCEPT
        {
            return is_scoped_;
        }

        /// Returns the calculated underlying type.
        const cpp_name& get_underlying_type() const STANDARDESE_NOEXCEPT
        {
            return type_calculated_;
        }

        /// Returns the type the programmer specified as in the source.
        const cpp_name& get_given_type() const STANDARDESE_NOEXCEPT
        {
            return type_given_;
        }

    private:
        cpp_enum(cpp_name scope, cpp_name name, cpp_comment comment)
        : cpp_type(std::move(scope), std::move(name), std::move(comment)),
          is_scoped_(false) {}

        cpp_name type_given_, type_calculated_;
        bool is_scoped_;

        friend parser;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_TYPE_HPP_INCLUDED
