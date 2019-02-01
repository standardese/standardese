// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
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
        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;

        text(std::string text) : text_(std::move(text)) {}

        std::string text_;
    };

    /// A fragment that is emphasized.
    ///
    /// This corresponds to the `<em>` HTML tag.
    class emphasis final : public phrasing_entity, public container_entity<phrasing_entity>
    {
    public:
        /// Builds an emphasis entity.
        class builder : public container_builder<emphasis>
        {
        public:
            /// \effects Creates it without any children.
            builder() : container_builder(std::unique_ptr<emphasis>(new emphasis())) {}
        };

        /// \returns A new emphasized text fragment.
        static std::unique_ptr<emphasis> build(std::string t)
        {
            builder b;
            b.add_child(text::build(std::move(t)));
            return b.finish();
        }

    private:
        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;

        emphasis() = default;
    };

    /// A fragment that is strongly emphasized.
    ///
    /// This corresponds to the `<strong>` HTML tag.
    class strong_emphasis final : public phrasing_entity, public container_entity<phrasing_entity>
    {
    public:
        /// Builds a strong emphasis entity.
        class builder : public container_builder<strong_emphasis>
        {
        public:
            /// \effects Creates it without any children.
            builder() : container_builder(std::unique_ptr<strong_emphasis>(new strong_emphasis()))
            {}
        };

        /// \returns A new strongly emphasized text fragment.
        static std::unique_ptr<strong_emphasis> build(std::string t)
        {
            builder b;
            b.add_child(text::build(std::move(t)));
            return b.finish();
        }

    private:
        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;

        strong_emphasis() = default;
    };

    /// A fragment that contains code.
    ///
    /// This corresponds to the `<code>` HTML tag.
    class code final : public phrasing_entity, public container_entity<phrasing_entity>
    {
    public:
        /// Builds a code entity.
        class builder : public container_builder<code>
        {
        public:
            /// \effects Creates it without any children.
            builder() : container_builder(std::unique_ptr<code>(new code())) {}
        };

        /// \returns A new code fragment containing the given text.
        static std::unique_ptr<code> build(std::string t)
        {
            builder b;
            b.add_child(text::build(std::move(t)));
            return b.finish();
        }

    private:
        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;

        code() = default;
    };

    /// A fragment that should be excluded in the output as-is.
    class verbatim final : public phrasing_entity
    {
    public:
        /// \returns A new verbatim fragment containing the given string.
        static std::unique_ptr<verbatim> build(std::string str)
        {
            return std::unique_ptr<verbatim>(new verbatim(std::move(str)));
        }

        const std::string& content() const noexcept
        {
            return str_;
        }

    private:
        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;

        explicit verbatim(std::string str) : str_(std::move(str)) {}

        std::string str_;
    };

    /// A soft line break.
    class soft_break final : public phrasing_entity
    {
    public:
        /// \returns A new soft break.
        static std::unique_ptr<soft_break> build()
        {
            return std::unique_ptr<soft_break>(new soft_break());
        }

    private:
        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;

        soft_break() noexcept = default;
    };

    /// A hard line break.
    class hard_break final : public phrasing_entity
    {
    public:
        /// \returns A new hard break.
        static std::unique_ptr<hard_break> build()
        {
            return std::unique_ptr<hard_break>(new hard_break());
        }

    private:
        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;

        hard_break() noexcept = default;
    };
} // namespace markup
} // namespace standardese

#endif // STANDARDESE_MARKUP_PHRASING_HPP_INCLUDED
