// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_OUTPUT_HPP_INCLUDED
#define STANDARDESE_OUTPUT_HPP_INCLUDED

#include <cstring>
#include <string>
#include <ostream>

#include <standardese/noexcept.hpp>
#include <standardese/output_stream.hpp>

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

    constexpr detail::newl_t newl;
    constexpr detail::blankl_t blankl;

    class output_base
    {
    public:
        output_base(output_stream_base &out) STANDARDESE_NOEXCEPT
        : output_(&out) {}

        output_base(const output_base &) = delete;
        output_base(output_base &&) = delete;

        virtual ~output_base() STANDARDESE_NOEXCEPT;

        output_base& operator=(const output_base&) = delete;
        output_base& operator=(output_base&&) = delete;

        enum class style
        {
            normal,
            italics,
            bold,
            code_span,
            count
        };

        class writer
        {
        public:
            writer(output_base &output) STANDARDESE_NOEXCEPT
            : output_(output) {}

            writer(const writer &) = delete;

            virtual ~writer() STANDARDESE_NOEXCEPT = default;

            writer& operator<<(const char *str)
            {
                output_.get_output().write_str(str, std::strlen(str));
                return *this;
            }

            writer& operator<<(const std::string &str)
            {
                output_.get_output().write_str(str.c_str(), str.size());
                return *this;
            }

            writer& operator<<(char c)
            {
                output_.get_output().write_char(c);
                return *this;
            }

            writer& operator<<(long long value)
            {
                return *this << std::to_string(value);
            }

            writer& operator<<(unsigned long long value)
            {
                return *this << std::to_string(value);
            }

            writer& operator<<(detail::newl_t)
            {
                output_.get_output().write_new_line();
                return *this;
            }

            writer& operator<<(detail::blankl_t)
            {
                output_.get_output().write_blank_line();
                return *this;
            }

            writer& operator<<(style st)
            {
                auto i = int(st);
                if (begin_[i])
                {
                    output_.write_begin(st);
                    begin_[i] = false;
                }
                else
                {
                    output_.write_end(st);
                    begin_[i] = true;
                }

                return *this;
            }

        protected:
            output_base &output_;

        private:
            bool begin_[int(style::count)] = {};
        };

        class paragraph_writer : public writer
        {
        public:
            paragraph_writer(output_base &output) STANDARDESE_NOEXCEPT
            : writer(output)
            {
                output_.write_paragraph_begin();
            }

            ~paragraph_writer() STANDARDESE_NOEXCEPT override
            {
                output_.write_paragraph_end();
            }

            void start_new() const
            {
                output_.write_paragraph_end();
                output_.write_paragraph_begin();
            }
        };

        class heading_writer : public writer
        {
        public:
            heading_writer(output_base &output, unsigned level) STANDARDESE_NOEXCEPT
            : writer(output), level_(level)
            {
                output_.write_header_begin(level_);
            }

            ~heading_writer() STANDARDESE_NOEXCEPT override
            {
                output_.write_header_end(level_);
            }

        private:
            unsigned level_;
        };

        void write_section_heading(const std::string &section_name)
        {
            do_write_section_heading(section_name);
        }

        class code_block_writer : public writer
        {
        public:
            code_block_writer(output_base &output) STANDARDESE_NOEXCEPT
            : writer(output)
            {
                output_.write_code_block_begin();
            }

            ~code_block_writer() STANDARDESE_NOEXCEPT
            {
                output_.write_code_block_end();
            }

            void indent(unsigned level)
            {
                output_.get_output().indent(level);
            }

            void unindent(unsigned level)
            {
                output_.get_output().unindent(level);
            }
        };

        void write_seperator() STANDARDESE_NOEXCEPT
        {
            do_write_seperator();
        }

    protected:
        output_stream_base& get_output() STANDARDESE_NOEXCEPT
        {
            return *output_;
        }

        virtual void write_header_begin(unsigned level) = 0;

        virtual void write_header_end(unsigned level)
        {
            write_header_begin(level);
        }

        virtual void do_write_seperator() = 0;

        virtual void write_begin(style s) = 0;

        virtual void write_end(style s)
        {
            write_begin(s);
        }

        virtual void write_paragraph_begin() = 0;

        virtual void write_paragraph_end()
        {
            write_paragraph_begin();
        }

        virtual void write_code_block_begin() = 0;
        virtual void write_code_block_end()
        {
            write_code_block_begin();
        }

        virtual void do_write_section_heading(const std::string &section_name) = 0;

    private:
        output_stream_base *output_;

        friend writer;
        friend paragraph_writer;
        friend heading_writer;
    };

    class markdown_output
    : public output_base
    {
    public:
        markdown_output(output_stream_base &output) STANDARDESE_NOEXCEPT
        : output_base(output) {}

    protected:
        void write_header_begin(unsigned level) override;
        void write_header_end(unsigned  level) override;

        void do_write_seperator() override;

        void write_begin(style s) override;

        void write_paragraph_begin() override;
        void write_paragraph_end() override;

        void write_code_block_begin() override;
        void write_code_block_end() override;

        void do_write_section_heading(const std::string &section_name) override;
    };
} // namespace standardese

#endif // STANDARDESE_OUTPUT_HPP_INCLUDED
