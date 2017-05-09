// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_LIST_HPP_INCLUDED
#define STANDARDESE_MARKUP_LIST_HPP_INCLUDED

#include <standardese/markup/block.hpp>
#include "phrasing.hpp"

namespace standardese
{
    namespace markup
    {
        /// The base class for all items in a list.
        class list_item_base : public block_entity
        {
        protected:
            using block_entity::block_entity;
        };

        /// An item in a list.
        ///
        /// This is the HTML `<li>` container.
        class list_item final : public list_item_base, public container_entity<block_entity>
        {
        public:
            /// Builds a list item.
            class builder : public container_builder<list_item>
            {
            public:
                /// \effects Builds an empty item.
                builder(block_id id = block_id())
                : container_builder(std::unique_ptr<list_item>(new list_item(std::move(id))))
                {
                }
            };

            /// \returns A list item consisting of the given block only.
            static std::unique_ptr<list_item> build(std::unique_ptr<block_entity> block)
            {
                return builder().add_child(std::move(block)).finish();
            }

        private:
            entity_kind do_get_kind() const noexcept override;

            list_item(block_id id) : list_item_base(std::move(id))
            {
            }
        };

        /// The term of a [standardese::markup::term_description_list_item]().
        class term final : public phrasing_entity, public container_entity<phrasing_entity>
        {
        public:
            /// Builds a new term.
            class builder : public container_builder<term>
            {
            public:
                /// \effects Creates an empty term.
                builder() : container_builder(std::unique_ptr<term>(new term))
                {
                }
            };

            /// \returns A built term consisting of the single entity.
            static std::unique_ptr<term> build(std::unique_ptr<phrasing_entity> entity)
            {
                return builder().add_child(std::move(entity)).finish();
            }

        private:
            entity_kind do_get_kind() const noexcept override;

            term() noexcept = default;
        };

        /// The description of a [standardese::markup::term_description_list_item]().
        class description final : public phrasing_entity, public container_entity<phrasing_entity>
        {
        public:
            /// Builds a description.
            class builder : public container_builder<description>
            {
            public:
                /// \effects Creates an empty description.
                builder() : container_builder(std::unique_ptr<description>(new description))
                {
                }
            };

            /// \returns A newly built description consisting only of the given phrasing entity.
            static std::unique_ptr<description> build(std::unique_ptr<phrasing_entity> phrasing)
            {
                return builder().add_child(std::move(phrasing)).finish();
            }

        private:
            entity_kind do_get_kind() const noexcept override;

            description() = default;
        };

        /// A list item that consists of a term and an description.
        ///
        /// It is a simple key-value pair.
        class term_description_item final : public list_item_base
        {
        public:
            /// \returns A newly built term description.
            static std::unique_ptr<term_description_item> build(
                block_id id, std::unique_ptr<markup::term> term,
                std::unique_ptr<markup::description> desc)
            {
                return std::unique_ptr<term_description_item>(
                    new term_description_item(std::move(id), std::move(term), std::move(desc)));
            }

            /// \returns The term.
            const markup::term& term() const noexcept
            {
                return *term_;
            }

            /// \returns The description.
            const markup::description& description() const noexcept
            {
                return *description_;
            }

        private:
            entity_kind do_get_kind() const noexcept override;

            term_description_item(block_id id, std::unique_ptr<markup::term> t,
                                  std::unique_ptr<markup::description> desc)
            : list_item_base(std::move(id)), term_(std::move(t)), description_(std::move(desc))
            {
            }

            std::unique_ptr<markup::term>        term_;
            std::unique_ptr<markup::description> description_;
        };

        /// An unordered list of items.
        class unordered_list final : public block_entity, public container_entity<list_item_base>
        {
        public:
            /// Builds an unordered list.
            class builder : public container_builder<unordered_list>
            {
            public:
                /// \effects Builds an empty list.
                builder(block_id id)
                : container_builder(
                      std::unique_ptr<unordered_list>(new unordered_list(std::move(id))))
                {
                }

                /// \effects Adds a new item.
                builder& add_item(std::unique_ptr<list_item_base> item)
                {
                    add_child(std::move(item));
                    return *this;
                }

            private:
                using container_builder::add_child;
            };

        private:
            entity_kind do_get_kind() const noexcept override;

            unordered_list(block_id id) : block_entity(std::move(id))
            {
            }
        };

        /// An ordered list of items.
        class ordered_list final : public block_entity, public container_entity<list_item_base>
        {
        public:
            /// Builds an ordered list.
            class builder : public container_builder<ordered_list>
            {
            public:
                /// \effects Builds an empty list.
                builder(block_id id)
                : container_builder(std::unique_ptr<ordered_list>(new ordered_list(std::move(id))))
                {
                }

                /// \effects Adds a new item.
                builder& add_item(std::unique_ptr<list_item_base> item)
                {
                    add_child(std::move(item));
                    return *this;
                }

            private:
                using container_builder::add_child;
            };

        private:
            entity_kind do_get_kind() const noexcept override;

            ordered_list(block_id id) : block_entity(std::move(id))
            {
            }
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_LIST_HPP_INCLUDED
