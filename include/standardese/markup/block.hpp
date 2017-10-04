// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_BLOCK_HPP_INCLUDED
#define STANDARDESE_MARKUP_BLOCK_HPP_INCLUDED

#include <type_safe/optional.hpp>

#include <standardese/markup/entity.hpp>

namespace standardese
{
    namespace markup
    {
        /// The file name of a [standardese::markup::document_entity]().
        ///
        /// It can either contain the extension already or need one.
        class output_name
        {
        public:
            /// \returns An `output_name` that still needs an extension.
            static output_name from_name(std::string name)
            {
                return output_name(std::move(name), true);
            }

            /// \returns An `output_name` that already has an extension.
            static output_name from_file_name(std::string file_name)
            {
                return output_name(std::move(file_name), false);
            }

            /// \returns The name of the file, may or may not contain the extension.
            const std::string& name() const noexcept
            {
                return name_;
            }

            /// \returns Whether or not it needs an extension.
            bool needs_extension() const noexcept
            {
                return needs_extension_;
            }

            /// \returns The file name of the output name, given the extension of the current format.
            std::string file_name(const char* format_extension) const
            {
                return needs_extension_ ? name() + "." + format_extension : name();
            }

        private:
            output_name(std::string name, bool need)
            : name_(std::move(name)), needs_extension_(need)
            {
            }

            std::string name_;
            bool        needs_extension_;
        };

        /// The id of a [standardese::markup::block_entity]().
        ///
        /// It must be unique and should only consist of alphanumerics or `-`.
        class block_id
        {
        public:
            /// \effects Creates an empty id.
            explicit block_id() : block_id("") {}

            /// \effects Creates it given the string representation.
            explicit block_id(std::string id) : id_(std::move(id)) {}

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

        /// A reference to a block specified by output name and id.
        ///
        /// It is used as the target of [standardese::markup::documentation_link](),
        /// for example.
        class block_reference
        {
        public:
            /// \effects Creates it giving output name and id.
            block_reference(output_name document, block_id id)
            : document_(std::move(document)), id_(std::move(id))
            {
            }

            /// \effects Creates it giving id only,
            /// the block is then in the same file as the entity that stores the reference.
            block_reference(block_id id) : id_(std::move(id)) {}

            /// \returns The output name of the document the block is in.
            /// If it does not have a document, the block is in the same document.
            const type_safe::optional<output_name>& document() const noexcept
            {
                return document_;
            }

            /// \returns The id of the block.
            const block_id& id() const noexcept
            {
                return id_;
            }

        private:
            type_safe::optional<output_name> document_;
            block_id                         id_;
        };

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
            block_entity(block_id id) : id_(std::move(id)) {}

        private:
            block_id id_;
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_BLOCK_HPP_INCLUDED
