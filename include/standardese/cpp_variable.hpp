// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_VARIABLE_HPP_INCLUDED
#define STANDARDESE_CPP_VARIABLE_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_type.hpp>

namespace standardese
{
    class cpp_variable_base : public cpp_entity
    {
    public:
        const cpp_type_ref& get_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

        const std::string& get_initializer() const STANDARDESE_NOEXCEPT
        {
            return initializer_;
        }

    protected:
        cpp_variable_base(cpp_entity::type t, cpp_cursor cur, md_ptr<md_comment> comment,
                          const cpp_entity& parent, cpp_type_ref type, std::string initializer)
        : cpp_entity(t, cur, std::move(comment), parent),
          type_(std::move(type)),
          initializer_(std::move(initializer))
        {
        }

    private:
        cpp_type_ref type_;
        std::string  initializer_;
    };

    class cpp_variable final : public cpp_variable_base
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::variable_t;
        }

        static cpp_ptr<cpp_variable> parse(translation_unit& tu, cpp_cursor cur,
                                           const cpp_entity& parent);

        bool is_thread_local() const STANDARDESE_NOEXCEPT
        {
            return thread_local_;
        }

    private:
        cpp_variable(cpp_cursor cur, md_ptr<md_comment> comment, const cpp_entity& parent,
                     cpp_type_ref type, std::string initializer, bool thr_local)
        : cpp_variable_base(get_entity_type(), cur, std::move(comment), parent, std::move(type),
                            std::move(initializer)),
          thread_local_(thr_local)
        {
        }

        bool thread_local_;

        friend detail::cpp_ptr_access;
    };

    class cpp_member_variable_base : public cpp_variable_base
    {
    public:
        static cpp_ptr<cpp_member_variable_base> parse(translation_unit& tu, cpp_cursor cur,
                                                       const cpp_entity& parent);

        bool is_mutable() const STANDARDESE_NOEXCEPT
        {
            return mutable_;
        }

    protected:
        cpp_member_variable_base(cpp_entity::type t, cpp_cursor cur, md_ptr<md_comment> comment,
                                 const cpp_entity& parent, cpp_type_ref type,
                                 std::string initializer, bool is_mut)
        : cpp_variable_base(t, cur, std::move(comment), parent, std::move(type),
                            std::move(initializer)),
          mutable_(is_mut)
        {
        }

    private:
        bool mutable_;
    };

    class cpp_member_variable final : public cpp_member_variable_base
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::member_variable_t;
        }

    private:
        cpp_member_variable(cpp_cursor cur, md_ptr<md_comment> comment, const cpp_entity& parent,
                            cpp_type_ref type, std::string initializer, bool is_mut)
        : cpp_member_variable_base(get_entity_type(), cur, std::move(comment), parent,
                                   std::move(type), std::move(initializer), is_mut)
        {
        }

        friend cpp_member_variable_base;
        friend detail::cpp_ptr_access;
    };

    class cpp_bitfield final : public cpp_member_variable_base
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::bitfield_t;
        }

        unsigned no_bits() const STANDARDESE_NOEXCEPT
        {
            return no_bits_;
        }

    private:
        cpp_bitfield(cpp_cursor cur, md_ptr<md_comment> comment, const cpp_entity& parent,
                     cpp_type_ref type, std::string initializer, unsigned no_bits, bool is_mut)
        : cpp_member_variable_base(get_entity_type(), cur, std::move(comment), parent,
                                   std::move(type), std::move(initializer), is_mut),
          no_bits_(no_bits)
        {
        }

        unsigned no_bits_;

        friend cpp_member_variable_base;
        friend detail::cpp_ptr_access;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_VARIABLE_HPP_INCLUDED
