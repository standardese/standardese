// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_QUOTE_HPP_INCLUDED
#define STANDARDESE_MARKUP_QUOTE_HPP_INCLUDED

#include <standardese/markup/block.hpp>

namespace standardese
{
    namespace markup
    {
        class blockquote final : public block_entity, public container_entity<block_entity>
        {
        public:
            /// Builds a new quote.
            class builder : public container_builder<blockquote>
            {
            public:
                /// \effects Creates an empty quote.
                builder(block_id id)
                : container_builder(std::unique_ptr<blockquote>(new blockquote(std::move(id))))
                {
                }
            };

        private:
            void do_append_html(std::string& result) const override;

            blockquote(block_id id) : block_entity(std::move(id))
            {
            }
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_QUOTE_HPP_INCLUDED
