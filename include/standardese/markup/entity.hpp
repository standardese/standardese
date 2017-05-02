// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_ENTITY_HPP_INCLUDED
#define STANDARDESE_MARKUP_ENTITY_HPP_INCLUDED

#include <type_safe/optional_ref.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace standardese
{
    namespace markup
    {
        enum class entity_kind;

        /// The base class for all markup entities.
        ///
        /// This is how the documentation output is described.
        /// An entity can be converted into various markup languages.
        class entity
        {
        public:
            entity(const entity&) = delete;
            entity& operator=(const entity&) = delete;
            virtual ~entity() noexcept       = default;

            /// \returns The kind of entity.
            entity_kind kind() const noexcept
            {
                return do_get_kind();
            }

            /// \returns A reference to the parent entity, if there is any.
            type_safe::optional_ref<const entity> parent() const noexcept
            {
                return parent_;
            }

        protected:
            entity() noexcept = default;

        private:
            /// \returns The kind of entity.
            virtual entity_kind do_get_kind() const noexcept = 0;

            type_safe::optional_ref<const entity> parent_;

            template <typename T>
            friend class container_entity;
        };

        /// A mix-in base class for entity that are a containers.
        ///
        /// It takes care of the container functions.
        /// \requires `T` must be derived from `entity`.
        template <typename T>
        class container_entity
        {
            using container = std::vector<std::unique_ptr<T>>;

        public:
            class iterator
            {
            public:
                using value_type        = const T;
                using reference         = const T&;
                using pointer           = const T*;
                using difference_type   = std::ptrdiff_t;
                using iterator_category = std::forward_iterator_tag;

                iterator() noexcept : cur_(nullptr)
                {
                }

                reference operator*() const noexcept
                {
                    return **cur_;
                }

                pointer operator->() const noexcept
                {
                    return cur_->get();
                }

                iterator& operator++() noexcept
                {
                    ++cur_;
                    return *this;
                }

                iterator operator++(int)noexcept
                {
                    auto tmp = *this;
                    ++(*this);
                    return tmp;
                }

                friend bool operator==(const iterator& a, const iterator& b) noexcept
                {
                    return a.cur_ == b.cur_;
                }

                friend bool operator!=(const iterator& a, const iterator& b) noexcept
                {
                    return !(a == b);
                }

            private:
                iterator(typename container::const_iterator cur) : cur_(cur)
                {
                }

                typename container::const_iterator cur_;

                friend container_entity;
            };

            /// \returns An iterator to the first child entity.
            iterator begin() const noexcept
            {
                return children_.begin();
            }

            /// \returns An iterator one past the last child entity.
            iterator end() const noexcept
            {
                return children_.end();
            }

        protected:
            ~container_entity() noexcept
            {
                static_assert(std::is_base_of<entity, T>::value, "T must be derived from entity");
            }

            /// Base class to create the builder.
            /// \requires `Derived` is the type derived from `entity_container`.
            template <typename Derived>
            class container_builder
            {
            public:
                /// \effects Adds a new child for the container.
                container_builder& add_child(std::unique_ptr<T> entity)
                {
                    static_cast<markup::entity&>(*entity).parent_ = type_safe::ref(*result_);
                    as_container().children_.push_back(std::move(entity));
                    return *this;
                }

                /// \returns The finished entity.
                std::unique_ptr<Derived> finish() noexcept
                {
                    return std::move(result_);
                }

            protected:
                /// \effects Creates it giving it the partially constructed entity.
                container_builder(std::unique_ptr<Derived> entity) noexcept
                : result_(std::move(entity))
                {
                    static_assert(std::is_base_of<container_entity<T>, Derived>::value,
                                  "Derived must be derived from container_entity");
                }

            private:
                container_entity<T>& as_container() const noexcept
                {
                    return *result_;
                }

                std::unique_ptr<Derived> result_;
            };

        private:
            container children_;
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_ENTITY_HPP_INCLUDED
