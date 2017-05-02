// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_LIST_HPP_INCLUDED
#define STANDARDESE_MARKUP_LIST_HPP_INCLUDED

#include <standardese/markup/block.hpp>

namespace standardese
{
    namespace markup
    {
        /// An item in a list.
        ///
        /// This is the HTML `<li>` container.
        class list_item final : public block_entity, public container_entity<block_entity>
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

        private:
            entity_kind do_get_kind() const noexcept override;

            list_item(block_id id) : block_entity(std::move(id))
            {
            }
        };

        /// An unordered list of items.
        class unordered_list final : public block_entity, public container_entity<list_item>
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

                /// \effects Adds a new [standardese::markup::list_item]() that only consists of the given block.
                builder& add_item(std::unique_ptr<block_entity> block)
                {
                    add_item(list_item::builder().add_child(std::move(block)).finish());
                    return *this;
                }

                /// \effects Adds a new item.
                builder& add_item(std::unique_ptr<list_item> item)
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
        class ordered_list final : public block_entity, public container_entity<list_item>
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

                /// \effects Adds a new [standardese::markup::list_item]() that only consists of the given block.
                builder& add_item(std::unique_ptr<block_entity> block)
                {
                    add_item(list_item::builder().add_child(std::move(block)).finish());
                    return *this;
                }

                /// \effects Adds a new item.
                builder& add_item(std::unique_ptr<list_item> item)
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
