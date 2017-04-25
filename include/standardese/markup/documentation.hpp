// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_DOCUMENTATION_HPP_INCLUDED
#define STANDARDESE_MARKUP_DOCUMENTATION_HPP_INCLUDED

#include <standardese/markup/block.hpp>
#include <standardese/markup/entity.hpp>

namespace standardese
{
    namespace markup
    {
        /// A generic container containing the documentation of some file.
        /// \notes This does not represent a stand-alone file, use a [standardese::markup::document_entity]() for that.
        class file_documentation final : public block_entity, public container_entity<block_entity>
        {
        public:
            /// Builds the documentation of a file.
            class builder : public container_builder<file_documentation>
            {
            public:
                /// \effects Creates it giving the id.
                builder(block_id id)
                : container_builder(
                      std::unique_ptr<file_documentation>(new file_documentation(std::move(id))))
                {
                }
            };

        private:
            void do_append_html(std::string& result) const override;

            file_documentation(block_id id) : block_entity(std::move(id))
            {
            }
        };

        /// A generic container containing the documentation of a single entity.
        /// \notes This does not represent the documentation of a file, use [standardese::markup::file_documentation]() for that.
        class entity_documentation final : public block_entity,
                                           public container_entity<block_entity>
        {
        public:
            /// Builds the documentation of an entity.
            class builder : public container_builder<entity_documentation>
            {
            public:
                /// \effects Creates it giving the id.
                /// \notes The id should be related to the name of the entity being documented here.
                builder(block_id id)
                : container_builder(std::unique_ptr<entity_documentation>(
                      new entity_documentation(std::move(id))))
                {
                }
            };

        private:
            void do_append_html(std::string& result) const override;

            entity_documentation(block_id id) : block_entity(std::move(id))
            {
            }
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_DOCUMENTATION_HPP_INCLUDED
