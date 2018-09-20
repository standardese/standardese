// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_LINK_HPP_INCLUDED
#define STANDARDESE_MARKUP_LINK_HPP_INCLUDED

#include <type_safe/variant.hpp>

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
        link_base(std::string title) : title_(std::move(title)) {}

    private:
        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::string title_;
    };

    /// An URL.
    class url
    {
    public:
        explicit url(std::string url) : str_(std::move(url)) {}

        const std::string& as_str() const noexcept
        {
            return str_;
        }

    private:
        std::string str_;
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
            builder(std::string title, markup::url u)
            : container_builder(
                  std::unique_ptr<external_link>(new external_link(std::move(title), std::move(u))))
            {}

            /// \effects Creates it giving the URL only.
            builder(markup::url url) : builder("", std::move(url)) {}
        };

        /// \returns The URL of the external location it links to.
        const markup::url& url() const noexcept
        {
            return url_;
        }

    private:
        entity_kind do_get_kind() const noexcept override;

        std::unique_ptr<entity> do_clone() const override;

        external_link(std::string title, markup::url url)
        : link_base(std::move(title)), url_(std::move(url))
        {}

        markup::url url_;
    };

    /// A link to another part of the documentation.
    ///
    /// Precisely, a link to another [standardese::markup::block_entity]() or some external URL.
    class documentation_link final : public link_base
    {
    public:
        /// Builds a documentation link.
        class builder : public container_builder<documentation_link>
        {
        public:
            /// \effects Creates it giving the title and unresolved destination.
            builder(std::string title, std::string dest)
            : container_builder(std::unique_ptr<documentation_link>(
                  new documentation_link(std::move(title), std::move(dest))))
            {}

            /// \effects Creates it giving the unresolved destination only.
            builder(std::string dest) : builder("", std::move(dest)) {}

            /// \effects Creates it giving the title and an internal destination.
            builder(std::string title, block_reference dest) : builder(std::move(title), "")
            {
                peek().resolve_destination(std::move(dest));
            }

        private:
            using container_builder::peek;

            friend documentation_link;
        };

        /// \returns The internal destination of the link, if it has been resolved to an internal
        /// destination.
        type_safe::optional_ref<const block_reference> internal_destination() const noexcept
        {
            return dest_.optional_value(type_safe::variant_type<block_reference>{});
        }

        /// \returns The external destination of the link, if it has been resolved to an external
        /// destination.
        type_safe::optional_ref<const markup::url> external_destination() const noexcept
        {
            return dest_.optional_value(type_safe::variant_type<markup::url>{});
        }

        /// \returns The unresolved destination id of the link, if it hasn't been resolved already.
        /// It might have been unresolved on purpose and should render just the content.
        type_safe::optional_ref<const std::string> unresolved_destination() const noexcept
        {
            return dest_.optional_value(type_safe::variant_type<std::string>{});
        }

        /// \effects Resolves the destination of the link.
        /// \notes This function is not thread safe.
        /// \group resolve
        void resolve_destination(block_reference ref) const
        {
            dest_ = std::move(ref);
        }

        /// \group resolve
        void resolve_destination(markup::url url) const
        {
            dest_ = std::move(url);
        }

    private:
        entity_kind do_get_kind() const noexcept override;

        std::unique_ptr<entity> do_clone() const override;

        documentation_link(std::string title, std::string dest)
        : link_base(std::move(title)), dest_(std::move(dest))
        {}

        mutable type_safe::variant<block_reference, markup::url, std::string> dest_;
    };
} // namespace markup
} // namespace standardese

#endif // STANDARDESE_MARKUP_LINK_HPP_INCLUDED
