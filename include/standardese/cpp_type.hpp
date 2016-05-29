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
        cpp_type(cpp_entity::type t, cpp_name scope, cpp_name name, CXType type)
        : cpp_entity(t, std::move(scope), std::move(name)),
          type_(type)
        {}

    private:
        CXType type_;
    };

    class cpp_type_ref
    {
    public:
        cpp_type_ref()
        : cpp_type_ref({}, "") {}

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
        static cpp_ptr<cpp_type_alias> parse(translation_unit &tu, const cpp_name &scope, cpp_cursor cur);

        cpp_type_alias(cpp_name scope, cpp_name name,
                       CXType type, cpp_type_ref target)
        : cpp_type(type_alias_t, std::move(scope), std::move(name), type),
          target_(std::move(target)) {}

        const cpp_type_ref& get_target() const STANDARDESE_NOEXCEPT
        {
            return target_;
        }

    private:
        cpp_type_ref target_;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_TYPE_HPP_INCLUDED
