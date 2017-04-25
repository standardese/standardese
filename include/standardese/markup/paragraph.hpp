// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_PARAGRAPH_HPP_INCLUDED
#define STANDARDESE_MARKUP_PARAGRAPH_HPP_INCLUDED

#include <standardese/markup/block.hpp>
#include <standardese/markup/phrasing.hpp>

namespace standardese
{
    namespace markup
    {
        /// A plain paragraph.
        class paragraph final : public block_entity, public container_entity<phrasing_entity>
        {
        public:
            /// Builds a paragraph.
            class builder : public container_builder<paragraph>
            {
            public:
                /// \effects Creates it giving the id.
                builder(block_id id = block_id())
                : container_builder(std::unique_ptr<paragraph>(new paragraph(std::move(id))))
                {
                }
            };

        private:
            void do_append_html(std::string& result) const override;

            paragraph(block_id id) : block_entity(std::move(id))
            {
            }
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_PARAGRAPH_HPP_INCLUDED
