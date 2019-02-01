// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_CODE_BLOCK_HPP_INCLUDED
#define STANDARDESE_MARKUP_CODE_BLOCK_HPP_INCLUDED

#include <standardese/markup/block.hpp>
#include <standardese/markup/phrasing.hpp>

namespace standardese
{
namespace markup
{
    class code_block final : public block_entity, public container_entity<phrasing_entity>
    {
        template <class Tag>
        class code_block_entity final : public phrasing_entity
        {
        public:
            static std::unique_ptr<code_block_entity<Tag>> build(std::string text)
            {
                return std::unique_ptr<code_block_entity<Tag>>(
                    new code_block_entity<Tag>(std::move(text)));
            }

            const std::string& string() const noexcept
            {
                return text_;
            }

        private:
            entity_kind do_get_kind() const noexcept override
            {
                return Tag::kind();
            }

            void do_visit(detail::visitor_callback_t, void*) const override {}

            std::unique_ptr<entity> do_clone() const override
            {
                return build(text_);
            }

            code_block_entity(std::string text) : text_(std::move(text)) {}

            std::string text_;
        };

        struct keyword_tag
        {
            static entity_kind kind() noexcept;
        };

        struct identifier_tag
        {
            static entity_kind kind() noexcept;
        };

        struct string_literal_tag
        {
            static entity_kind kind() noexcept;
        };

        struct int_literal_tag
        {
            static entity_kind kind() noexcept;
        };

        struct float_literal_tag
        {
            static entity_kind kind() noexcept;
        };

        struct punctuation_tag
        {
            static entity_kind kind() noexcept;
        };

        struct preprocessor_tag
        {
            static entity_kind kind() noexcept;
        };

    public:
        /// A special [standardese::markup::text]() entity that represents a certain syntax
        /// highlighting.
        ///
        /// The HTML renderer will use a `<span>` block with classes as used in [code
        /// prettify](https://github.com/google/code-prettify). In particular:
        /// * keywords will have the `kwd` class
        /// * identifiers will have `typ`, `dec`, `var` and `fun`
        /// * string literals will have the `str` class
        /// * int and float literals will have the `lit` class
        /// * punctuation will have the `pun` class
        /// * preprocessor tokens will have the `pre` class (not in code prettify themes)
        ///
        /// \notes These entities are only meant to be used in a code block.
        /// \notes These HTML classes are the only ones not prefixed with `standardese-`.
        /// \group code_block_entity code_block syntaxh highlighting entities
        using keyword = code_block_entity<keyword_tag>;
        /// \group code_block_entity
        using identifier = code_block_entity<identifier_tag>;
        /// \group code_block_entity
        using string_literal = code_block_entity<string_literal_tag>;
        /// \group code_block_entity
        using int_literal = code_block_entity<int_literal_tag>;
        /// \group code_block_entity
        using float_literal = code_block_entity<float_literal_tag>;
        /// \group code_block_entity
        using punctuation = code_block_entity<punctuation_tag>;
        /// \group code_block_entity
        using preprocessor = code_block_entity<preprocessor_tag>;

        /// Builds a code block.
        class builder : public container_builder<code_block>
        {
        public:
            /// \effects Creates an empty code block.
            builder(block_id id, std::string lang)
            : container_builder(
                  std::unique_ptr<code_block>(new code_block(std::move(id), std::move(lang))))
            {}
        };

        /// \returns A new code block containing only the given string.
        static std::unique_ptr<code_block> build(block_id id, std::string language,
                                                 std::string code)
        {
            return builder(std::move(id), std::move(language))
                .add_child(text::build(std::move(code)))
                .finish();
        }

        /// \returns The language of the code block.
        const std::string& language() const noexcept
        {
            return lang_;
        }

    private:
        code_block(block_id id, std::string lang)
        : block_entity(std::move(id)), lang_(std::move(lang))
        {}

        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;

        std::string lang_;
    };
} // namespace markup
} // namespace standardese

#endif // STANDARDESE_MARKUP_CODE_BLOCK_HPP_INCLUDED
