// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_HEADING_HPP_INCLUDED
#define STANDARDESE_MARKUP_HEADING_HPP_INCLUDED

#include <standardese/markup/block.hpp>
#include <standardese/markup/phrasing.hpp>

namespace standardese
{
    namespace markup
    {
        /// A heading.
        ///
        /// By default, this corresponds to LaTeX `\paragraph` or HTML level 4,
        /// but this can change, depending on the context it is in.
        class heading final : public block_entity, public container_entity<phrasing_entity>
        {
        public:
            /// Builds a heading.
            class builder : public container_builder<heading>
            {
            public:
                /// \effects Creates an empty heading given its id.
                builder(block_id id)
                : container_builder(std::unique_ptr<heading>(new heading(std::move(id))))
                {
                }
            };

            /// Builds a heading containing the given string as text.
            static std::unique_ptr<heading> build(block_id id, std::string text)
            {
                return builder(id).add_child(text::build(std::move(text))).finish();
            }

        private:
            entity_kind do_get_kind() const noexcept override;

            void do_visit(detail::visitor_callback_t cb, void* mem) const override;

            heading(block_id id) : block_entity(std::move(id))
            {
            }
        };

        /// A subheading.
        ///
        /// This corresponds to LaTeX `\subparagraph` or HTML level 5.
        class subheading final : public block_entity, public container_entity<phrasing_entity>
        {
        public:
            /// Builds a heading.
            class builder : public container_builder<subheading>
            {
            public:
                /// \effects Creates an empty heading given its id.
                builder(block_id id)
                : container_builder(std::unique_ptr<subheading>(new subheading(std::move(id))))
                {
                }
            };

            /// Builds a subheading containing the given string as text.
            static std::unique_ptr<subheading> build(block_id id, std::string text)
            {
                return builder(id).add_child(text::build(std::move(text))).finish();
            }

        private:
            entity_kind do_get_kind() const noexcept override;

            void do_visit(detail::visitor_callback_t cb, void* mem) const override;

            subheading(block_id id) : block_entity(std::move(id))
            {
            }
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_HEADING_HPP_INCLUDED
