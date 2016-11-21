// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_OUTPUT_HPP_INCLUDED
#define STANDARDESE_OUTPUT_HPP_INCLUDED

#include <cstring>
#include <string>
#include <ostream>

#include <standardese/md_blocks.hpp>
#include <standardese/noexcept.hpp>
#include <standardese/output_format.hpp>
#include <standardese/output_stream.hpp>
#include <standardese/string.hpp>

namespace spdlog
{
    class logger;
} // namespace spdlog

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

    class code_block_writer
    {
    public:
        code_block_writer(const md_entity& parent) STANDARDESE_NOEXCEPT : parent_(&parent)
        {
        }

        md_ptr<md_code_block> get_code_block(const char* fence = "cpp")
        {
            return md_code_block::make(*parent_, stream_.get_string().c_str(), fence);
        }

        void indent(unsigned width)
        {
            stream_.indent(width);
        }

        void unindent(unsigned width)
        {
            stream_.unindent(width);
        }

        void remove_trailing_line()
        {
            stream_.remove_trailing_line();
        }

        code_block_writer& fill(std::size_t size, char c)
        {
            for (std::size_t i = 0u; i != size; ++i)
                stream_.write_char(c);
            return *this;
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

        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
        code_block_writer& operator<<(T value)
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
        string_output    stream_;
        const md_entity* parent_;
    };

    class md_document;
    class index;

    void resolve_urls(const std::shared_ptr<spdlog::logger>& logger, const index& i,
                      md_document& document, const char* extension);

    using path = std::string;

    class output
    {
    public:
        output(const index& i, path prefix, output_format_base& format)
        : prefix_(std::move(prefix)), format_(&format), index_(&i)
        {
        }

        void render(const std::shared_ptr<spdlog::logger>& logger, const md_document& document,
                    const char* output_extension = nullptr);

        output_format_base& get_format() STANDARDESE_NOEXCEPT
        {
            return *format_;
        }

        const path& get_prefix() const STANDARDESE_NOEXCEPT
        {
            return prefix_;
        }

    private:
        path                prefix_;
        output_format_base* format_;
        const index*        index_;
    };
} // namespace standardese

#endif // STANDARDESE_OUTPUT_HPP_INCLUDED
