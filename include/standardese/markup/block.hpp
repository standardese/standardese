// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_BLOCK_HPP_INCLUDED
#define STANDARDESE_MARKUP_BLOCK_HPP_INCLUDED

#include <standardese/markup/entity.hpp>

namespace standardese
{
    namespace markup
    {
        /// The id of a [standardese::markup::block_entity]().
        ///
        /// It must be unique and should only consist of alphanumerics or `-`.
        class block_id
        {
        public:
            /// \effects Creates an empty id.
            explicit block_id() : block_id("")
            {
            }

            /// \effects Creates it given the string representation.
            explicit block_id(std::string id) : id_(std::move(id))
            {
            }

            /// \returns Whether or not the id is empty.
            bool empty() const noexcept
            {
                return id_.empty();
            }

            /// \returns The string representation of the id.
            const std::string& as_str() const noexcept
            {
                return id_;
            }

        private:
            std::string id_;
        };

        /// \returns Whether or not two ids are (un-)equal.
        /// \group block_id_equal block_id comparison
        inline bool operator==(const block_id& a, const block_id& b) noexcept
        {
            return a.as_str() == b.as_str();
        }

        /// \group block_id_equal
        inline bool operator!=(const block_id& a, const block_id& b) noexcept
        {
            return !(a == b);
        }

        /// Base class for all block entities.
        ///
        /// A block entity is part of the structure of the document.
        class block_entity : public entity
        {
        public:
            /// \returns The unique id of the block.
            const block_id& id() const noexcept
            {
                return id_;
            }

        protected:
            /// \effects Creates it giving the id.
            block_entity(block_id id) : id_(std::move(id))
            {
            }

        private:
            block_id id_;
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_BLOCK_HPP_INCLUDED
