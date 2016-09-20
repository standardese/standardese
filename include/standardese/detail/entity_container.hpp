// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_ENTITY_CONTAINER_HPP_INCLUDED
#define STANDARDESE_DETAIL_ENTITY_CONTAINER_HPP_INCLUDED

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

#include <standardese/noexcept.hpp>
#include <standardese/string.hpp>

namespace standardese
{
    namespace detail
    {
        template <class T, class Base, template <typename> class Ptr>
        class entity_container
        {
            static_assert(std::is_base_of<Base, T>::value, "T must be derived from Base");

        public:
            entity_container() STANDARDESE_NOEXCEPT : first_(nullptr), last_(nullptr)
            {
            }

            bool empty() const STANDARDESE_NOEXCEPT
            {
                return first_ == nullptr;
            }

            class iterator
            {
            public:
                using value_type        = T;
                using reference         = T&;
                using pointer           = T*;
                using difference_type   = std::ptrdiff_t;
                using iterator_category = std::forward_iterator_tag;

                iterator() STANDARDESE_NOEXCEPT : cur_(nullptr)
                {
                }

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

                iterator operator++(int)STANDARDESE_NOEXCEPT
                {
                    auto tmp = *this;
                    ++(*this);
                    return tmp;
                }

                friend bool operator==(const iterator& a, const iterator& b) STANDARDESE_NOEXCEPT
                {
                    return a.cur_ == b.cur_;
                }

                friend bool operator!=(const iterator& a, const iterator& b) STANDARDESE_NOEXCEPT
                {
                    return !(a == b);
                }

            private:
                iterator(Base* ptr) : cur_(static_cast<pointer>(ptr))
                {
                }

                T* cur_;

                friend entity_container;
            };

            class const_iterator
            {
            public:
                using value_type        = T;
                using reference         = const T&;
                using pointer           = const T*;
                using difference_type   = std::ptrdiff_t;
                using iterator_category = std::forward_iterator_tag;

                const_iterator() STANDARDESE_NOEXCEPT : cur_(nullptr)
                {
                }

                reference operator*() const STANDARDESE_NOEXCEPT
                {
                    return *cur_;
                }

                pointer operator->() const STANDARDESE_NOEXCEPT
                {
                    return cur_;
                }

                const_iterator& operator++() STANDARDESE_NOEXCEPT
                {
                    cur_ = static_cast<pointer>(cur_->next_.get());
                    return *this;
                }

                const_iterator operator++(int)STANDARDESE_NOEXCEPT
                {
                    auto tmp = *this;
                    ++(*this);
                    return tmp;
                }

                friend bool operator==(const const_iterator& a,
                                       const const_iterator& b) STANDARDESE_NOEXCEPT
                {
                    return a.cur_ == b.cur_;
                }

                friend bool operator!=(const const_iterator& a,
                                       const const_iterator& b) STANDARDESE_NOEXCEPT
                {
                    return !(a == b);
                }

            private:
                const_iterator(Base* ptr) : cur_(static_cast<pointer>(ptr))
                {
                }

                const T* cur_;

                friend entity_container;
            };

            iterator begin() STANDARDESE_NOEXCEPT
            {
                return iterator(first_.get());
            }

            iterator end() STANDARDESE_NOEXCEPT
            {
                return {};
            }

            const_iterator begin() const STANDARDESE_NOEXCEPT
            {
                return const_iterator(first_.get());
            }

            const_iterator end() const STANDARDESE_NOEXCEPT
            {
                return {};
            }

        protected:
            ~entity_container() STANDARDESE_NOEXCEPT = default;

            void add_entity(Ptr<Base> entity)
            {
                if (!entity)
                    return;

                if (last_)
                {
                    last_->next_ = std::move(entity);
                    last_        = last_->next_.get();
                }
                else
                {
                    first_ = std::move(entity);
                    last_  = first_.get();
                }
            }

            Ptr<Base> remove_entity_after(Base* base)
            {
                auto& next = base ? base->next_ : first_;
                assert(next);

                auto entity = std::move(next);
                next        = std::move(entity->next_);
                if (last_ == entity.get())
                {
                    assert(next == nullptr);
                    last_ = base;
                }

                return std::move(entity);
            }

            Base* get_last() STANDARDESE_NOEXCEPT
            {
                return last_;
            }

            const Base* get_last() const STANDARDESE_NOEXCEPT
            {
                return last_;
            }

        private:
            Ptr<Base> first_;
            Base*     last_;
        };
    }
} // namespace standardese::detail

#endif // STANDARDESE_DETAIL_ENTITY_CONTAINER_HPP_INCLUDED
