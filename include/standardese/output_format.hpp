// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_OUTPUT_FORMAT_HPP_INCLUDED
#define STANDARDESE_OUTPUT_FORMAT_HPP_INCLUDED

#include <standardese/noexcept.hpp>
#include "output_stream.hpp"

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

        void write_code_block_begin(output_stream_base& output)
        {
            do_write_code_block(output, true);
        }

        void write_code_block_end(output_stream_base& output)
        {
            do_write_code_block(output, false);
        }

    protected:
        output_format_base() STANDARDESE_NOEXCEPT = default;

    private:
        virtual void do_render(output_stream_base& output, const md_entity& entity) = 0;

        virtual void do_write_code_block(output_stream_base& output, bool begin) = 0;
    };

    class output_format_markdown : public output_format_base
    {
    public:
        explicit output_format_markdown(unsigned width = 100) : width_(width)
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
        void do_render(output_stream_base& output, const md_entity& entity);
        void do_write_code_block(output_stream_base& output, bool begin);

        unsigned width_;
    };
} // namespace standardeses

#endif // STANDARDESE_OUTPUT_FORMAT_HPP_INCLUDED
