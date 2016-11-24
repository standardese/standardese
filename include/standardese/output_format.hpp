// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_OUTPUT_FORMAT_HPP_INCLUDED
#define STANDARDESE_OUTPUT_FORMAT_HPP_INCLUDED

#include <standardese/noexcept.hpp>
#include "output_stream.hpp"
#include "md_entity.hpp"

namespace standardese
{
    class output_stream_base;
    class md_entity;

    class output_format_base
    {
    public:
        output_format_base(const output_format_base&) = delete;
        output_format_base(output_format_base&&)      = delete;

        virtual ~output_format_base() STANDARDESE_NOEXCEPT;

        output_format_base& operator=(const output_format_base&) = delete;
        output_format_base& operator=(output_format_base&&) = delete;

        void render(output_stream_base& output, const md_entity& entity)
        {
            do_render(output, entity);
        }

        const char* extension() const STANDARDESE_NOEXCEPT
        {
            return get_extension();
        }

    protected:
        output_format_base() STANDARDESE_NOEXCEPT = default;

    private:
        virtual void do_render(output_stream_base& output, const md_entity& entity) = 0;

        virtual const char* get_extension() const STANDARDESE_NOEXCEPT = 0;
    };

    class output_format_xml : public output_format_base
    {
    public:
        static const char* name() STANDARDESE_NOEXCEPT
        {
            return "xml";
        }

    private:
        void do_render(output_stream_base& output, const md_entity& entity) override;

        const char* get_extension() const STANDARDESE_NOEXCEPT override
        {
            return "xml";
        }
    };

    class output_format_html : public output_format_base
    {
    public:
        static const char* name() STANDARDESE_NOEXCEPT
        {
            return "html";
        }

    private:
        void do_render(output_stream_base& output, const md_entity& entity) override;

        const char* get_extension() const STANDARDESE_NOEXCEPT override
        {
            return "html";
        }
    };

    namespace detail
    {
        constexpr unsigned default_width = 100;
    } // namespace detail

    class output_format_markdown : public output_format_base
    {
    public:
        static const char* name() STANDARDESE_NOEXCEPT
        {
            return "commonmark";
        }

        explicit output_format_markdown(unsigned width = detail::default_width) : width_(width)
        {
        }

        unsigned get_width() const STANDARDESE_NOEXCEPT
        {
            return width_;
        }

        void set_width(unsigned w) STANDARDESE_NOEXCEPT
        {
            width_ = w;
        }

    private:
        void do_render(output_stream_base& output, const md_entity& entity) override;

        const char* get_extension() const STANDARDESE_NOEXCEPT override
        {
            return "md";
        }

        unsigned width_;
    };

    class output_format_man : public output_format_base
    {
    public:
        static const char* name() STANDARDESE_NOEXCEPT
        {
            return "man";
        }

        explicit output_format_man(unsigned width = detail::default_width) : width_(width)
        {
        }

        unsigned get_width() const STANDARDESE_NOEXCEPT
        {
            return width_;
        }

        void set_width(unsigned w) STANDARDESE_NOEXCEPT
        {
            width_ = w;
        }

    private:
        void do_render(output_stream_base& output, const md_entity& entity) override;

        const char* get_extension() const STANDARDESE_NOEXCEPT override
        {
            return "man";
        }

        unsigned width_;
    };

    class output_format_latex : public output_format_base
    {
    public:
        static const char* name() STANDARDESE_NOEXCEPT
        {
            return "latex";
        }

        explicit output_format_latex(unsigned width = detail::default_width) : width_(width)
        {
        }

        unsigned get_width() const STANDARDESE_NOEXCEPT
        {
            return width_;
        }

        void set_width(unsigned w) STANDARDESE_NOEXCEPT
        {
            width_ = w;
        }

    private:
        void do_render(output_stream_base& output, const md_entity& entity) override;

        const char* get_extension() const STANDARDESE_NOEXCEPT override
        {
            return "tex";
        }

        unsigned width_;
    };

    std::unique_ptr<output_format_base> make_output_format(const std::string& name,
                                                           unsigned width = detail::default_width);
} // namespace standardeses

#endif // STANDARDESE_OUTPUT_FORMAT_HPP_INCLUDED
