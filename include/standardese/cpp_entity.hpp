// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_ENTITY_HPP_INCLUDED
#define STANDARDESE_CPP_ENTITY_HPP_INCLUDED

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

#include <standardese/cpp_cursor.hpp>
#include <standardese/noexcept.hpp>
#include <standardese/string.hpp>

namespace standardese
{
    template <typename T>
    class cpp_entity_container;
    class translation_unit;

    using cpp_name = string;
    using cpp_raw_comment = string;

    template <typename T>
    using cpp_ptr = std::unique_ptr<T>;

    class cpp_entity;

    using cpp_entity_ptr = cpp_ptr<cpp_entity>;

    class cpp_entity
    {
    public:
        enum type
        {
            file_t,

            inclusion_directive_t,
            macro_definition_t,

            namespace_t,
            namespace_alias_t,
            using_directive_t,
            using_declaration_t,

            type_alias_t,

            enum_t,
            signed_enum_value_t,
            unsigned_enum_value_t,

            variable_t,
            member_variable_t,
            bitfield_t,

            function_parameter_t,
            function_t,
            member_function_t,
            conversion_op_t,
            constructor_t,
            destructor_t,

            template_type_parameter_t,
            non_type_template_parameter_t,
            template_template_parameter_t,

            function_template_t,
            function_template_specialization_t,

            class_t,
            class_template_t,
            class_template_partial_specialization_t,
            class_template_full_specialization_t,

            base_class_t,
            access_specifier_t,

            invalid_t
        };

        static cpp_entity_ptr try_parse(translation_unit &tu, cpp_cursor cur, const cpp_entity &parent);

        cpp_entity(cpp_entity &&) = delete;

        cpp_entity(const cpp_entity &) = delete;

        virtual ~cpp_entity() STANDARDESE_NOEXCEPT = default;

        cpp_entity &operator=(const cpp_entity &) = delete;

        cpp_entity &operator=(cpp_entity &&) = delete;

        /// \returns The name of the entity as specified in the source.
        virtual cpp_name get_name() const;

        /// \returns The scope of the entity, without trailing `::`, empty for global scope.
        virtual cpp_name get_scope() const
        {
            if (!parent_ || parent_->get_entity_type() == file_t)
                return "";
            return parent_->get_full_name();
        }

        /// \returns The full name of the entity, scope followed by name.
        cpp_name get_full_name() const
        {
            auto scope = get_scope();
            return scope.empty() ? get_name() : std::string(scope.c_str()) + "::" + get_name().c_str();
        }

        /// \returns The raw comment string.
        cpp_raw_comment get_comment() const;

        /// \returns The type of the entity.
        type get_entity_type() const STANDARDESE_NOEXCEPT
        {
            return t_;
        }

        /// \returns The libclang cursor of the declaration of the entity.
        cpp_cursor get_cursor() const STANDARDESE_NOEXCEPT
        {
            return cursor_;
        }

        bool has_parent() const STANDARDESE_NOEXCEPT
        {
            return parent_ != nullptr;
        }

        /// \returns The parent entity,
        /// that is the entity which owns the current one.
        /// \requires The entity has a parent.
        const cpp_entity& get_parent() const STANDARDESE_NOEXCEPT
        {
            return *parent_;
        }

    protected:
        cpp_entity(type t, cpp_cursor cur, const cpp_entity &parent) STANDARDESE_NOEXCEPT
        : cursor_(cur), next_(nullptr), parent_(&parent), t_(t) {}

        /// \effects Creates it without a parent.
        cpp_entity(type t, cpp_cursor cur) STANDARDESE_NOEXCEPT
        : cursor_(cur), next_(nullptr), parent_(nullptr), t_(t) {}

    private:
        cpp_cursor cursor_;
        std::unique_ptr<cpp_entity> next_;
        const cpp_entity *parent_;
        type t_;

        template <typename T>
        friend class cpp_entity_container;
    };

    template <typename T>
    class cpp_entity_container
    {
        static_assert(std::is_base_of<cpp_entity, T>::value, "T must be derived from cpp_entity");
    public:
        ~cpp_entity_container() STANDARDESE_NOEXCEPT = default;

        bool empty() const STANDARDESE_NOEXCEPT
        {
            return first_ == nullptr;
        }

        class iterator
        {
        public:
            using value_type = T;
            using reference = const T&;
            using pointer = const T*;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;

            iterator() STANDARDESE_NOEXCEPT
            : cur_(nullptr) {}

            reference operator*() const STANDARDESE_NOEXCEPT
            {
                return *cur_;
            }

            pointer operator->() const STANDARDESE_NOEXCEPT
            {
                return cur_;
            }

            iterator& operator++() STANDARDESE_NOEXCEPT
            {
                cur_ = static_cast<pointer>(cur_->next_.get());
                return *this;
            }

            iterator operator++(int) STANDARDESE_NOEXCEPT
            {
                auto tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator==(const iterator &a, const iterator &b) STANDARDESE_NOEXCEPT
            {
                return a.cur_ == b.cur_;
            }

            friend bool operator!=(const iterator &a, const iterator &b) STANDARDESE_NOEXCEPT
            {
                return !(a == b);
            }

        private:
            iterator(cpp_entity *ptr)
            : cur_(static_cast<pointer>(ptr)) {}

            const T *cur_;

            friend cpp_entity_container;
        };

        iterator begin() const STANDARDESE_NOEXCEPT
        {
            return iterator(first_.get());
        }

        iterator end() const STANDARDESE_NOEXCEPT
        {
            return {};
        }

    protected:
        cpp_entity_container() STANDARDESE_NOEXCEPT
        : first_(nullptr), last_(nullptr) {}

        void add_entity(cpp_ptr<T> entity)
        {
            if (!entity)
                return;

            if (last_)
            {
                last_->next_ = std::move(entity);
                last_ = last_->next_.get();
            }
            else
            {
                first_ = std::move(entity);
                last_ = first_.get();
            }
        }

    private:
        cpp_entity_ptr first_;
        cpp_entity *last_;
    };

    namespace detail
    {
        struct cpp_ptr_access
        {
            template <typename T, typename ... Args>
            static cpp_ptr<T> make(Args&&... args)
            {
                return cpp_ptr<T>(new T(std::forward<Args>(args)...));
            }
        };

        template <typename T, typename ... Args>
        cpp_ptr<T> make_ptr(Args&&... args)
        {
            return cpp_ptr_access::make<T>(std::forward<Args>(args)...);
        }

        template <typename Derived>
        cpp_ptr<Derived> downcast(cpp_entity_ptr entity)
        {
            auto ptr = entity.release();
            return cpp_ptr<Derived>(static_cast<Derived*>(ptr));
        }
    } // namespace detail
} // namespace standardese

#endif // STANDARDESE_CPP_ENTITY_HPP_INCLUDED
