// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_ENTITY_HPP_INCLUDED
#define STANDARDESE_CPP_ENTITY_HPP_INCLUDED

#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>

#include <standardese/noexcept.hpp>

namespace standardese
{
    template <typename T>
    class cpp_entity_container;

    class parser;
    class translation_unit;

    using cpp_name = std::string;
    using cpp_raw_comment = std::string;

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
            enum_value_t,
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

        cpp_entity(cpp_entity&&) = delete;
        cpp_entity(const cpp_entity&) = delete;

        virtual ~cpp_entity() STANDARDESE_NOEXCEPT = default;

        cpp_entity& operator=(const cpp_entity&) = delete;
        cpp_entity& operator=(cpp_entity&&) = delete;

        const cpp_name& get_name() const STANDARDESE_NOEXCEPT
        {
            return name_;
        }

        virtual cpp_name get_unique_name() const
        {
            return scope_.empty() ? name_ : scope_ + "::" + name_;
        }

        // excluding trailing "::"
        const cpp_name& get_scope() const STANDARDESE_NOEXCEPT
        {
            return scope_;
        }

        const cpp_raw_comment& get_comment() const STANDARDESE_NOEXCEPT
        {
            return comment_;
        }

        type get_entity_type() const STANDARDESE_NOEXCEPT
        {
            return t_;
        }

    protected:
        cpp_entity(type t, cpp_name scope, cpp_name n, cpp_raw_comment c) STANDARDESE_NOEXCEPT
        : name_(std::move(n)), scope_(std::move(scope)), comment_(std::move(c)),
          next_(nullptr), t_(t)
        {}

        void set_name(cpp_name n)
        {
            name_ = std::move(n);
        }

        void set_type(type t) STANDARDESE_NOEXCEPT
        {
            t_ = t;
        }

    private:
        cpp_name name_, scope_;
        cpp_raw_comment comment_;

        std::unique_ptr<cpp_entity> next_;

        type t_;

        template <typename T>
        friend class cpp_entity_container;
    };

    template <typename T>
    using cpp_ptr = std::unique_ptr<T>;

    namespace detail
    {
        template <typename T, typename ... Args>
        cpp_ptr<T> make_ptr(Args&&... args)
        {
            return cpp_ptr<T>(new T(std::forward<Args>(args)...));
        }
    } // namespace detail

    using cpp_entity_ptr = std::unique_ptr<cpp_entity>;

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

    class parser;

    /// Required interface for parser classes.
    class cpp_entity_parser
    {
    public:
        /// Returns the fully parsed entity.
        /// Parser is given for final registration.
        virtual cpp_entity_ptr finish(const parser &par) = 0;

        /// Adds a new entity, should forward to cpp_entity_container.
        virtual void add_entity(cpp_entity_ptr)
        {}

        /// Returns the name of the current scope if container
        virtual cpp_name scope_name()
        {
            return "";
        }
    };

    class cpp_parameter_base
    : public cpp_entity
    {
    protected:
        cpp_parameter_base(cpp_entity::type t, cpp_name name, cpp_raw_comment comment)
        : cpp_entity(t, "", std::move(name), std::move(comment)) {}
    };
} // namespace standardese

#endif // STANDARDESE_CPP_ENTITY_HPP_INCLUDED
