// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_LINK_HPP_INCLUDED
#define STANDARDESE_MARKUP_LINK_HPP_INCLUDED

#include <standardese/markup/block.hpp>
#include <standardese/markup/phrasing.hpp>

namespace standardese
{
    namespace markup
    {
        /// Base class for all links.
        ///
        /// The actual link text will be stored as the children.
        class link_base : public phrasing_entity, public container_entity<phrasing_entity>
        {
        public:
            /// \returns The title of the link.
            const std::string& title() const noexcept
            {
                return title_;
            }

        protected:
            /// \effects Sets the title of the link.
            link_base(std::string title) : title_(std::move(title))
            {
            }

        private:
            std::string title_;
        };

        /// A link to some external URL.
        class external_link final : public link_base
        {
        public:
            /// Builds an external link.
            class builder : public container_builder<external_link>
            {
            public:
                /// \effects Creates it giving the title and URL.
                builder(std::string title, std::string url)
                : container_builder(std::unique_ptr<external_link>(
                      new external_link(std::move(title), std::move(url))))
                {
                }

                /// \effects Creates it giving the URL only.
                builder(std::string url) : builder("", std::move(url))
                {
                }
            };

            /// \returns The URL of the external location it links to.
            const std::string& url() const noexcept
            {
                return url_;
            }

        private:
            void do_append_html(std::string& result) const override;

            external_link(std::string title, std::string url)
            : link_base(std::move(title)), url_(std::move(url))
            {
            }

            std::string url_;
        };

        /// A link to another part of the documentation.
        ///
        /// Precisely, a link to another [standardese::markup::block_entity]().
        class internal_link final : public link_base
        {
        public:
            /// Builds an internal link.
            class builder : public container_builder<internal_link>
            {
            public:
                /// \effects Creates it giving the title and destination.
                builder(std::string title, block_reference dest)
                : container_builder(std::unique_ptr<internal_link>(
                      new internal_link(std::move(title), std::move(dest))))
                {
                }

                /// \effects Creates it giving the destination only.
                builder(block_reference dest) : builder("", std::move(dest))
                {
                }
            };

            /// \returns The destination of the link.
            const block_reference& destination() const noexcept
            {
                return dest_;
            }

        private:
            void do_append_html(std::string& result) const override;

            internal_link(std::string title, block_reference dest)
            : link_base(std::move(title)), dest_(std::move(dest))
            {
            }

            block_reference dest_;
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_LINK_HPP_INCLUDED
