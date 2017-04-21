// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_PHRASING_HPP_INCLUDED
#define STANDARDESE_MARKUP_PHRASING_HPP_INCLUDED

#include <standardese/markup/entity.hpp>

namespace standardese
{
    namespace markup
    {
        /// Base class for all phrasing entities.
        ///
        /// A pharsing entity adds structural information like emphasis to text fragments.
        class phrasing_entity : public entity
        {
        protected:
            phrasing_entity() noexcept = default;
        };

        /// A normal text fragment.
        class text final : public phrasing_entity
        {
        public:
            /// \returns A new text fragment containing the given text.
            static std::unique_ptr<text> build(std::string t)
            {
                return std::unique_ptr<text>(new text(std::move(t)));
            }

            /// \returns The text of the text fragment.
            const std::string& string() const noexcept
            {
                return text_;
            }

        private:
            void do_append_html(std::string& result) const override;

            text(std::string text) : text_(std::move(text))
            {
            }

            std::string text_;
        };

        /// A fragment that is emphasized.
        ///
        /// This corresponds to the `<em>` HTML tag.
        class emphasis final : public phrasing_entity, public entity_container<phrasing_entity>
        {
        public:
            /// Builds an emphasis entity.
            class builder : public container_builder<emphasis>
            {
            public:
                /// \effects Creates it without any children.
                builder() : container_builder(std::unique_ptr<emphasis>(new emphasis()))
                {
                }
            };

            /// \returns A new emphasized text fragment.
            static std::unique_ptr<emphasis> build(std::string t)
            {
                builder b;
                b.add_child(text::build(std::move(t)));
                return b.finish();
            }

        private:
            void do_append_html(std::string& result) const override;

            emphasis() noexcept = default;
        };

        /// A fragment that is strongly emphasized.
        ///
        /// This corresponds to the `<strong>` HTML tag.
        class strong_emphasis final : public phrasing_entity,
                                      public entity_container<phrasing_entity>
        {
        public:
            /// Builds a strong emphasis entity.
            class builder : public container_builder<strong_emphasis>
            {
            public:
                /// \effects Creates it without any children.
                builder()
                : container_builder(std::unique_ptr<strong_emphasis>(new strong_emphasis()))
                {
                }
            };

            /// \returns A new strongly emphasized text fragment.
            static std::unique_ptr<strong_emphasis> build(std::string t)
            {
                builder b;
                b.add_child(text::build(std::move(t)));
                return b.finish();
            }

        private:
            void do_append_html(std::string& result) const override;

            strong_emphasis() noexcept = default;
        };

        /// A fragment that is a definition of a term.
        ///
        /// This corresponds to the `<dfn>` HTML tag.
        class definition final : public phrasing_entity, public entity_container<phrasing_entity>
        {
        public:
            /// Builds a definition entity.
            class builder : public container_builder<definition>
            {
            public:
                /// \effects Creates it without any children.
                builder() : container_builder(std::unique_ptr<definition>(new definition()))
                {
                }
            };

            /// \returns A new definition containing the given text.
            static std::unique_ptr<definition> build(std::string t)
            {
                builder b;
                b.add_child(text::build(std::move(t)));
                return b.finish();
            }

        private:
            void do_append_html(std::string& result) const override;

            definition() noexcept = default;
        };

        /// A fragment that contains code.
        ///
        /// This corresponds to the `<code>` HTML tag.
        class code final : public phrasing_entity, public entity_container<phrasing_entity>
        {
        public:
            /// Builds a code entity.
            class builder : public container_builder<code>
            {
            public:
                /// \effects Creates it without any children.
                builder() : container_builder(std::unique_ptr<code>(new code()))
                {
                }
            };

            /// \returns A new code fragment containing the given text.
            static std::unique_ptr<code> build(std::string t)
            {
                builder b;
                b.add_child(text::build(std::move(t)));
                return b.finish();
            }

        private:
            void do_append_html(std::string& result) const override;

            code() noexcept = default;
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_PHRASING_HPP_INCLUDED
