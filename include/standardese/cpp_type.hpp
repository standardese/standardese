// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_TYPE_HPP_INCLUDED
#define STANDARDESE_CPP_TYPE_HPP_INCLUDED

#include <standardese/cpp_cursor.hpp>
#include <standardese/cpp_entity.hpp>

namespace standardese
{
    class parser;

    struct cpp_cursor;

    class cpp_type
    : public cpp_entity
    {
    public:
        CXType get_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

    protected:
        cpp_type(cpp_name scope, cpp_name name, cpp_comment comment, CXType type)
        : cpp_entity(std::move(scope), std::move(name), std::move(comment)),
          type_(type)
        {}

    private:
        CXType type_;
    };

    class cpp_type_ref
    {
    public:
        cpp_type_ref(CXType type, cpp_name given)
        : given_(std::move(given)), type_(type) {}

        /// Returns the name as specified in the source.
        const cpp_name& get_name() const STANDARDESE_NOEXCEPT
        {
            return given_;
        }

        /// Returns the full name with all scopes as libclang returns it.
        cpp_name get_full_name() const;

        /// Returns the libclang target type.
        CXType get_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

    private:
        cpp_name given_;
        CXType type_;
    };

    class cpp_type_alias
    : public cpp_type
    {
    public:
        static cpp_ptr<cpp_type_alias> parse(const parser &p, const cpp_name &scope, cpp_cursor cur);

        cpp_type_alias(cpp_name scope, cpp_name name, cpp_comment comment,
                       CXType type, cpp_type_ref target)
        : cpp_type(std::move(scope), std::move(name), std::move(comment), type),
          target_(std::move(target)) {}

        const cpp_type_ref& get_target() const STANDARDESE_NOEXCEPT
        {
            return target_;
        }

    private:
        cpp_type_ref target_;
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

        const cpp_type_ref& get_underlying_type() const STANDARDESE_NOEXCEPT
        {
            return underlying_;
        }

    private:
        cpp_enum(cpp_name scope, cpp_name name, cpp_comment comment,
                 CXType type, cpp_type_ref underlying)
        : cpp_type(std::move(scope), std::move(name), std::move(comment), type),
          underlying_(std::move(underlying)),
          is_scoped_(false) {}

        cpp_type_ref underlying_;
        bool is_scoped_;

        friend parser;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_TYPE_HPP_INCLUDED
