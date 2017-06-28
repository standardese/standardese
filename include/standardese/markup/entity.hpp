// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_ENTITY_HPP_INCLUDED
#define STANDARDESE_MARKUP_ENTITY_HPP_INCLUDED

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include <type_safe/optional_ref.hpp>

#include <standardese/markup/visitor.hpp>

namespace standardese
{
    namespace markup
    {
        enum class entity_kind;

        /// \exclude
        namespace detail
        {
            struct parent_updater;
        } // namespace detail

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

            /// \returns A copy of itself.
            std::unique_ptr<entity> clone() const
            {
                return do_clone();
            }

        protected:
            entity() noexcept = default;

        private:
            /// \returns The kind of entity.
            virtual entity_kind do_get_kind() const noexcept = 0;

            /// \effects Invokes the callback for all children.
            virtual void do_visit(detail::visitor_callback_t cb, void* mem) const = 0;

            /// \returns A copy of itself.
            virtual std::unique_ptr<entity> do_clone() const = 0;

            type_safe::optional_ref<const entity> parent_;

            friend detail::parent_updater;
            friend void detail::call_visit(const entity& e, visitor_callback_t cb, void* mem);
        };

        /// \exclude
        namespace detail
        {
            template <typename U, typename T>
            std::unique_ptr<U> unchecked_downcast(std::unique_ptr<T> ptr)
            {
                return std::unique_ptr<U>(static_cast<U*>(ptr.release()));
            }
        } // namespace detail

        /// \returns A copy of the given entity.
        /// \requires `T` must be derived from [standardese::markup::entity]().
        template <typename T>
        std::unique_ptr<T> clone(const T& obj)
        {
            static_assert(std::is_base_of<entity, T>::value, "must be derived from entity");
            return std::unique_ptr<T>(static_cast<T*>(obj.clone().release()));
        }

        /// \exclude
        namespace detail
        {
            template <typename T>
            class vector_ptr_iterator
            {
                using container = std::vector<std::unique_ptr<T>>;

            public:
                using value_type        = const T;
                using reference         = const T&;
                using pointer           = const T*;
                using difference_type   = std::ptrdiff_t;
                using iterator_category = std::forward_iterator_tag;

                vector_ptr_iterator() noexcept : cur_(nullptr)
                {
                }

                vector_ptr_iterator(typename container::const_iterator cur) : cur_(cur)
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

                vector_ptr_iterator& operator++() noexcept
                {
                    ++cur_;
                    return *this;
                }

                vector_ptr_iterator operator++(int)noexcept
                {
                    auto tmp = *this;
                    ++(*this);
                    return tmp;
                }

                friend bool operator==(const vector_ptr_iterator& a,
                                       const vector_ptr_iterator& b) noexcept
                {
                    return a.cur_ == b.cur_;
                }

                friend bool operator!=(const vector_ptr_iterator& a,
                                       const vector_ptr_iterator& b) noexcept
                {
                    return !(a == b);
                }

            private:
                typename container::const_iterator cur_;
            };

            template <typename T>
            struct vector_ptr_range
            {
                vector_ptr_iterator<T> begin_, end_;

                const vector_ptr_iterator<T>& begin() const noexcept
                {
                    return begin_;
                }

                const vector_ptr_iterator<T>& end() const noexcept
                {
                    return end_;
                }
            };

            struct parent_updater
            {
                static void set(entity& e, type_safe::object_ref<const entity> parent)
                {
                    e.parent_ = parent;
                }
            };
        } // namespace detail

        /// A mix-in base class for entity that are a containers.
        ///
        /// It takes care of the container functions.
        /// \requires `T` must be derived from `entity`.
        template <typename T>
        class container_entity
        {
            using container = std::vector<std::unique_ptr<T>>;

        public:
            using iterator = detail::vector_ptr_iterator<T>;

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
                    detail::parent_updater::set(*entity, type_safe::ref(*result_));
                    as_container().children_.push_back(std::move(entity));
                    return *this;
                }

                /// \returns Whether or not the container is empty.
                bool empty() noexcept
                {
                    return result_->begin() == result_->end();
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

                /// \returns The not yet finished entity.
                Derived& peek() noexcept
                {
                    return *result_;
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
