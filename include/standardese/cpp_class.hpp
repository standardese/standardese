// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_CLASS_HPP_INCLUDED
#define STANDARDESE_CPP_CLASS_HPP_INCLUDED

#include <standardese/cpp_type.hpp>

namespace standardese
{
    enum cpp_access_specifier_t
    {
        cpp_private,
        cpp_protected,
        cpp_public
    };

    const char* to_string(cpp_access_specifier_t access) STANDARDESE_NOEXCEPT;

    class cpp_access_specifier final
    : public cpp_entity
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::access_specifier_t;
        }

        static cpp_ptr<cpp_access_specifier> parse(translation_unit &tu,
                                                   cpp_cursor cur, const cpp_entity &parent);

        cpp_name get_name() const override
        {
            return to_string(access_);
        }

        cpp_access_specifier_t get_access() const STANDARDESE_NOEXCEPT
        {
            return access_;
        }

    private:
        cpp_access_specifier(cpp_cursor cur, const cpp_entity &parent,
                             cpp_access_specifier_t a)
        : cpp_entity(get_entity_type(), cur, parent),
          access_(a) {}

        cpp_access_specifier_t access_;

        friend detail::cpp_ptr_access;
    };

    class cpp_base_class final
    : public cpp_entity
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::base_class_t;
        }

        static cpp_ptr<cpp_base_class> parse(translation_unit &tu,
                                             cpp_cursor cur, const cpp_entity &parent);

        cpp_name get_name() const override;

        const cpp_type_ref& get_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

        cpp_access_specifier_t get_access() const STANDARDESE_NOEXCEPT
        {
            return access_;
        }

        bool is_virtual() const STANDARDESE_NOEXCEPT
        {
            return virtual_;
        }

    private:
        cpp_base_class(cpp_cursor cur, const cpp_entity &parent,
                       cpp_type_ref type, cpp_access_specifier_t a,
                       bool virt)
        : cpp_entity(get_entity_type(), cur, parent),
          type_(std::move(type)), access_(a), virtual_(virt) {}

        cpp_type_ref type_;
        cpp_access_specifier_t access_;
        bool virtual_;

        friend detail::cpp_ptr_access;
    };

    enum cpp_class_type
    {
        cpp_class_t,
        cpp_struct_t,
        cpp_union_t
    };

    class cpp_class
    : public cpp_type, public cpp_entity_container<cpp_entity>
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::class_t;
        }

        static cpp_ptr<cpp_class> parse(translation_unit &tu,
                                        cpp_cursor cur, const cpp_entity &parent);

        void add_entity(cpp_entity_ptr e)
        {
            cpp_entity_container::add_entity(std::move(e));
        }

        cpp_class_type get_class_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

        bool is_final() const STANDARDESE_NOEXCEPT
        {
            return final_;
        }

    private:
        cpp_class(cpp_cursor cur, const cpp_entity &parent,
                  cpp_class_type t, bool is_final)
        : cpp_type(get_entity_type(), cur, parent),
          type_(t), final_(is_final) {}

        cpp_class_type type_;
        bool final_;

        friend detail::cpp_ptr_access;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_CLASS_HPP_INCLUDED
