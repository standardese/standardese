// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_OUTPUT_HPP_INCLUDED
#define STANDARDESE_OUTPUT_HPP_INCLUDED

#include <cstring>
#include <string>
#include <ostream>

#include <spdlog/fmt/fmt.h>

#include <standardese/md_blocks.hpp>
#include <standardese/md_custom.hpp>
#include <standardese/noexcept.hpp>
#include <standardese/output_format.hpp>
#include <standardese/output_stream.hpp>
#include <standardese/string.hpp>
#include <standardese/template_processor.hpp>

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
        code_block_writer(const md_entity& parent, bool use_advanced) STANDARDESE_NOEXCEPT
            : parent_(&parent),
              use_advanced_(use_advanced)
        {
        }

        md_entity_ptr get_code_block(const char* fence = "cpp")
        {
            if (use_advanced_)
                return md_code_block_advanced::make(*parent_, stream_.get_string().c_str(), fence);
            else
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

        code_block_writer& fill_ws(std::size_t size)
        {
            for (std::size_t i = 0u; i != size; ++i)
                stream_.write_char(' ');
            return *this;
        }

        code_block_writer& operator<<(const char* str)
        {
            write_str_escaped(str, std::strlen(str));
            return *this;
        }

        code_block_writer& operator<<(const std::string& str)
        {
            write_str_escaped(str.c_str(), str.size());
            return *this;
        }

        code_block_writer& operator<<(const string& str)
        {
            write_str_escaped(str.c_str(), str.length());
            return *this;
        }

        code_block_writer& write_link(bool top_level, const string& str, const string& unique_name)
        {
            if (str.empty())
                return *this;
            else if (!use_advanced_ || top_level || unique_name.empty())
                return *this << str;
            write_c_str(
                fmt::format("<a href='standardese://{}/'>{}</a>", unique_name.c_str(), str.c_str())
                    .c_str());
            return *this;
        }

        code_block_writer& operator<<(char c)
        {
            write_str_escaped(&c, 1);
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
        void write_c_str(const char* str)
        {
            stream_.write_str(str, std::strlen(str));
        }

        void write_str_escaped(const char* str, std::size_t size)
        {
            if (!use_advanced_)
                stream_.write_str(str, size);
            else
                for (auto ptr = str; ptr != str + size; ++ptr)
                {
                    if (*ptr == '&')
                        write_c_str("&amp;");
                    else if (*ptr == '<')
                        write_c_str("&lt;");
                    else if (*ptr == '>')
                        write_c_str("&gt;");
                    else
                        stream_.write_char(*ptr);
                }
        }

        string_output    stream_;
        const md_entity* parent_;
        bool             use_advanced_;
    };

    class md_document;
    class index;
    class linker;
    struct documentation;
    class doc_entity;

    using path = std::string;

    void normalize_urls(const index& idx, md_container& doc,
                        const doc_entity* default_context = nullptr);

    struct raw_document
    {
        path        file_name;
        path        file_extension;
        std::string text;

        raw_document() = default;

        raw_document(path file_name, std::string text);
    };

    class output
    {
    public:
        output(const parser& p, const index& i, path prefix, output_format_base& format)
        : prefix_(std::move(prefix)), format_(&format), parser_(&p), index_(&i)
        {
        }

        void render(const std::shared_ptr<spdlog::logger>& logger, const md_document& document,
                    const char* output_extension = nullptr);

        void render_template(const std::shared_ptr<spdlog::logger>& logger,
                             const template_file& templ, const documentation& doc,
                             const char* output_extension = nullptr);

        void render_raw(const std::shared_ptr<spdlog::logger>& logger, const raw_document& document,
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
        const parser*       parser_;
        const index*        index_;
    };
} // namespace standardese

#endif // STANDARDESE_OUTPUT_HPP_INCLUDED
