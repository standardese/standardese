// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_ENTITY_KIND_HPP_INCLUDED
#define STANDARDESE_MARKUP_ENTITY_KIND_HPP_INCLUDED

namespace standardese
{
    namespace markup
    {
        /// The kind of markup entity.
        enum class entity_kind
        {
            main_document,
            subdocument,
            template_document,

            file_documentation,
            entity_documentation,

            heading,
            subheading,

            paragraph,

            list_item,

            term,
            description,
            term_description_item,

            unordered_list,
            ordered_list,

            block_quote,

            code_block,
            code_block_keyword,
            code_block_identifier,
            code_block_string_literal,
            code_block_int_literal,
            code_block_float_literal,
            code_block_punctuation,
            code_block_preprocessor,

            brief_section,
            details_section,
            inline_section,
            list_section,

            thematic_break,

            text,
            emphasis,
            strong_emphasis,
            code,

            external_link,
            internal_link,
        };

        /// \returns Whether or not the entity is a phrasing entity,
        /// that is, derived from [standardese::markup::phrasing_entity]().
        bool is_phrasing(entity_kind kind) noexcept;

        /// \returns Whether or not the entity is a block,
        /// that is, derived from [standardese::markup::block_entity]().
        bool is_block(entity_kind kind) noexcept;
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_ENTITY_KIND_HPP_INCLUDED
