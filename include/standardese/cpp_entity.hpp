// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_ENTITY_HPP_INCLUDED
#define STANDARDESE_CPP_ENTITY_HPP_INCLUDED

#include <cstddef>
#include <iterator>
#include <memory>
#include <string>

#include <standardese/noexcept.hpp>

namespace standardese
{
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

        virtual cpp_name get_unique_name() const STANDARDESE_NOEXCEPT
        {
            return get_name();
        }

        const cpp_comment& get_comment() const STANDARDESE_NOEXCEPT
        {
            return comment_;
        }

    protected:
        cpp_entity(cpp_name n, cpp_comment c) STANDARDESE_NOEXCEPT
        : name_(std::move(n)), comment_(std::move(c)),
          next_(nullptr)
        {}

        void set_name(cpp_name n)
        {
            name_ = std::move(n);
        }

    private:
        cpp_name name_;
        cpp_comment comment_;

        std::unique_ptr<cpp_entity> next_;

        friend class cpp_entity_container;
    };

    using cpp_entity_ptr = std::unique_ptr<cpp_entity>;

    class cpp_entity_container
    {
    public:
        bool empty() const STANDARDESE_NOEXCEPT
        {
            return first_ == nullptr;
        }

        class iterator
        {
        public:
            using value_type = cpp_entity;
            using reference = const cpp_entity&;
            using pointer = const cpp_entity*;
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
                cur_ = cur_->next_.get();
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
            iterator(const cpp_entity *ptr)
            : cur_(ptr) {}

            const cpp_entity *cur_;

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

        void add_entity(cpp_entity_ptr entity)
        {
            do_add_entity(std::move(entity));
        }

    protected:
        cpp_entity_container() STANDARDESE_NOEXCEPT
        : first_(nullptr), last_(nullptr) {}

        ~cpp_entity_container() STANDARDESE_NOEXCEPT = default;

    private:
        virtual void do_add_entity(cpp_entity_ptr cur)
        {
            if (last_)
            {
                last_->next_ = std::move(cur);
                last_ = last_->next_.get();
            }
            else
            {
                first_ = std::move(cur);
                last_ = first_.get();
            }
        }

        cpp_entity_ptr first_;
        cpp_entity *last_;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_ENTITY_HPP_INCLUDED
