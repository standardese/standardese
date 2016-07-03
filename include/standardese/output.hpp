// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_OUTPUT_HPP_INCLUDED
#define STANDARDESE_OUTPUT_HPP_INCLUDED

#include <cstring>
#include <string>
#include <ostream>

#include <standardese/noexcept.hpp>
#include <standardese/output_format.hpp>
#include <standardese/output_stream.hpp>
#include <standardese/string.hpp>
#include "md_blocks.hpp"

namespace standardese
{
    namespace detail
    {
        struct newl_t
        {
            constexpr newl_t(){};
        };
        struct blankl_t
        {
            constexpr blankl_t(){};
        };
    }

    constexpr detail::newl_t   newl;
    constexpr detail::blankl_t blankl;

    class output
    {
    public:
        output(output_stream_base& stream, output_format_base& format) STANDARDESE_NOEXCEPT
            : stream_(&stream),
              format_(&format)
        {
        }

        void render(const md_entity& entity)
        {
            format_->render(*stream_, entity);
        }

        class code_block_writer
        {
        public:
            code_block_writer(output& out, const md_entity& parent) STANDARDESE_NOEXCEPT
                : output_(out),
                  parent_(&parent)
            {
            }

            code_block_writer(const code_block_writer&) = delete;

            ~code_block_writer() STANDARDESE_NOEXCEPT_IF(false)
            {
                output_.render(*md_code_block::make(*parent_, stream_.get_string().c_str(), "cpp"));
            }

            void indent(unsigned width)
            {
                stream_.indent(width);
            }

            void unindent(unsigned width)
            {
                stream_.unindent(width);
            }

            code_block_writer& operator<<(const char* str)
            {
                stream_.write_str(str, std::strlen(str));
                return *this;
            }

            code_block_writer& operator<<(const std::string& str)
            {
                stream_.write_str(str.c_str(), str.size());
                return *this;
            }

            code_block_writer& operator<<(const string& str)
            {
                stream_.write_str(str.c_str(), str.length());
                return *this;
            }

            code_block_writer& operator<<(char c)
            {
                stream_.write_char(c);
                return *this;
            }

            code_block_writer& operator<<(long long value)
            {
                return *this << std::to_string(value);
            }

            code_block_writer& operator<<(unsigned long long value)
            {
                return *this << std::to_string(value);
            }

            code_block_writer& operator<<(detail::newl_t)
            {
                stream_.write_new_line();
                return *this;
            }

            code_block_writer& operator<<(detail::blankl_t)
            {
                stream_.write_blank_line();
                return *this;
            }

        private:
            output&          output_;
            string_output    stream_;
            const md_entity* parent_;
        };

        output_stream_base& get_output() STANDARDESE_NOEXCEPT
        {
            return *stream_;
        }

        output_format_base& get_format() STANDARDESE_NOEXCEPT
        {
            return *format_;
        }

    private:
        output_stream_base* stream_;
        output_format_base* format_;
    };
} // namespace standardese

#endif // STANDARDESE_OUTPUT_HPP_INCLUDED
