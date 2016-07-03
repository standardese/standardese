// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_OUTPUT_STREAM_HPP_INCLUDED
#define STANDARDESE_OUTPUT_STREAM_HPP_INCLUDED

#include <cassert>
#include <fstream>
#include <ostream>

#include <standardese/noexcept.hpp>

namespace standardese
{
    class output_stream_base
    {
    public:
        output_stream_base() = default;

        output_stream_base(const output_stream_base&) = delete;
        output_stream_base(output_stream_base&&)      = delete;

        virtual ~output_stream_base() STANDARDESE_NOEXCEPT;

        output_stream_base& operator=(const output_stream_base&) = delete;
        output_stream_base& operator=(output_stream_base&&) = delete;

        void write_str(const char* str, std::size_t n);

        void write_char(char c);

        /// Starts a new line.
        void write_new_line()
        {
            if (last_ != '\n')
                write_char('\n');
        }

        /// Writes a blank line.
        void write_blank_line()
        {
            write_new_line();
            write_char('\n');
        }

        void indent(unsigned width);

        void unindent(unsigned width);

    private:
        virtual void do_write_char(char c) = 0;

        void do_indent();

        char     last_  = 0;
        unsigned level_ = 0;
    };

    class streambuf_output : public output_stream_base
    {
    public:
        streambuf_output(std::streambuf& buf) STANDARDESE_NOEXCEPT : buffer_(&buf)
        {
        }

        streambuf_output(std::ostream& out) STANDARDESE_NOEXCEPT : buffer_(out.rdbuf())
        {
            assert(buffer_);
        }

    private:
        void do_write_char(char c) override
        {
            buffer_->sputc(c);
        }

        std::streambuf* buffer_;
    };

    class file_output : public output_stream_base
    {
    public:
        file_output(const std::string& file) STANDARDESE_NOEXCEPT : file_(file)
        {
            assert(file_.is_open());
        }

    private:
        void do_write_char(char c) override
        {
            file_.rdbuf()->sputc(c);
        }

        std::ofstream file_;
    };

    class string_output : public output_stream_base
    {
    public:
        string_output() STANDARDESE_NOEXCEPT
        {
        }

        const std::string& get_string() const STANDARDESE_NOEXCEPT
        {
            return str_;
        }

    private:
        void do_write_char(char c) override
        {
            str_.push_back(c);
        }

        std::string str_;
    };
} // namespace standardese

#endif // STANDARDESE_OUTPUT_STREAM_HPP_INCLUDED
