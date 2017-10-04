// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_THEMATIC_BREAK_HPP
#define STANDARDESE_MARKUP_THEMATIC_BREAK_HPP

#include <standardese/markup/block.hpp>

namespace standardese
{
    namespace markup
    {
        /// A thematic break.
        class thematic_break final : public block_entity
        {
        public:
            /// \returns A new thematic break.
            static std::unique_ptr<thematic_break> build()
            {
                return std::unique_ptr<thematic_break>(new thematic_break());
            }

        private:
            entity_kind do_get_kind() const noexcept override;

            void do_visit(detail::visitor_callback_t cb, void* mem) const override;

            std::unique_ptr<entity> do_clone() const override;

            thematic_break() : block_entity(block_id()) {}
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_THEMATIC_BREAK_HPP
