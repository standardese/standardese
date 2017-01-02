// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_TYPE_HPP_INCLUDED
#define STANDARDESE_CPP_TYPE_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_entity_registry.hpp>
#include <clang-c/Index.h>

namespace standardese
{
    class cpp_type : public cpp_entity
    {
    public:
        CXType get_type() const STANDARDESE_NOEXCEPT;

    protected:
        cpp_type(type t, cpp_cursor cur, const cpp_entity& parent) : cpp_entity(t, cur, parent)
        {
        }
    };

    class cpp_type_ref
    {
    public:
        cpp_type_ref(cpp_name name, CXType type);

        const cpp_name& get_name() const STANDARDESE_NOEXCEPT
        {
            return name_;
        }

        cpp_name get_full_name() const STANDARDESE_NOEXCEPT;

        CXType get_cxtype() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

        CXType get_underlying_cxtype() const STANDARDESE_NOEXCEPT;

        cpp_cursor get_declaration() const STANDARDESE_NOEXCEPT;

        const cpp_entity* get(const cpp_entity_registry& e) const STANDARDESE_NOEXCEPT
        {
            return e.try_lookup(get_declaration());
        }

    private:
        cpp_name name_;
        CXType   type_;
    };

    cpp_cursor get_declaration(CXType type) STANDARDESE_NOEXCEPT;

    inline bool operator==(const cpp_type_ref& a, const cpp_type_ref& b) STANDARDESE_NOEXCEPT
    {
        return clang_equalTypes(a.get_cxtype(), b.get_cxtype()) == 1u;
    }

    inline bool operator!=(const cpp_type_ref& a, const cpp_type_ref& b) STANDARDESE_NOEXCEPT
    {
        return !(a == b);
    }

    class cpp_type_alias final : public cpp_type
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::type_alias_t;
        }

        static cpp_ptr<cpp_type_alias> parse(translation_unit& tu, cpp_cursor cur,
                                             const cpp_entity& parent);

        const cpp_type_ref& get_target() const STANDARDESE_NOEXCEPT
        {
            return target_;
        }

        bool is_templated() const STANDARDESE_NOEXCEPT;

    private:
        bool is_semantic_parent() const STANDARDESE_NOEXCEPT override
        {
            return !is_templated();
        }

        cpp_type_alias(cpp_cursor cur, const cpp_entity& parent, cpp_type_ref target)
        : cpp_type(get_entity_type(), cur, parent), target_(target)
        {
        }

        cpp_type_ref target_;

        friend detail::cpp_ptr_access;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_TYPE_HPP_INCLUDED
