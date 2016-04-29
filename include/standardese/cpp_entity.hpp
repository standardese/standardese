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

    using cpp_name = std::string;
    using cpp_comment = std::string;

    class cpp_entity
    {
    public:
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

        const cpp_comment& get_comment() const STANDARDESE_NOEXCEPT
        {
            return comment_;
        }

    protected:
        cpp_entity(cpp_name scope, cpp_name n, cpp_comment c) STANDARDESE_NOEXCEPT
        : name_(std::move(n)), scope_(std::move(scope)), comment_(std::move(c)),
          next_(nullptr)
        {}

        void set_name(cpp_name n)
        {
            name_ = std::move(n);
        }

    private:
        cpp_name name_, scope_;
        cpp_comment comment_;

        std::unique_ptr<cpp_entity> next_;

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

        ~cpp_entity_container() STANDARDESE_NOEXCEPT = default;

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
} // namespace standardese

#endif // STANDARDESE_CPP_ENTITY_HPP_INCLUDED
