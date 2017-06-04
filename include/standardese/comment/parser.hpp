// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_PARSER_HPP_INCLUDED
#define STANDARDESE_COMMENT_PARSER_HPP_INCLUDED

#include <stdexcept>
#include <vector>

#include <type_safe/optional.hpp>
#include <type_safe/variant.hpp>

#include <standardese/markup/doc_section.hpp>

#include <standardese/comment/config.hpp>
#include <standardese/comment/metadata.hpp>

extern "C" {
typedef struct cmark_node   cmark_node;
typedef struct cmark_parser cmark_parser;
}

namespace standardese
{
    namespace comment
    {
        /// The CommonMark parser.
        ///
        /// This is just a RAII wrapper over the `cmark_parser`
        /// and the [standardese::comment::config]().
        class parser
        {
        public:
            /// \effects Creates a new parser using the given configuration.
            parser(comment::config c = comment::config());

            ~parser() noexcept;

            parser(const parser&) = delete;
            parser& operator=(const parser&) = delete;

            /// \returns The parser.
            cmark_parser* get() const noexcept
            {
                return parser_;
            }

            /// \returns The config.
            const comment::config& config() const noexcept
            {
                return config_;
            }

        private:
            comment::config config_;
            cmark_parser*   parser_;
        };

        /// The root of the CommonMark AST.
        ///
        /// This is just a RAII wrapper over the `cmark_node`.
        class ast_root
        {
        public:
            /// \effects Takes ownership of the given node.
            /// It will call `cmark_node_free()` in the destructor.
            explicit ast_root(cmark_node* root) : root_(root)
            {
            }

            ast_root(ast_root&& other) noexcept : root_(other.root_)
            {
                other.root_ = nullptr;
            }

            ~ast_root() noexcept;

            ast_root& operator=(ast_root&& other) noexcept
            {
                ast_root tmp(std::move(other));
                std::swap(root_, tmp.root_);
                return *this;
            }

            /// \returns The node.
            cmark_node* get() const noexcept
            {
                return root_;
            }

        private:
            cmark_node* root_;
        };

        /// Reads the CommonMark AST from the comment text.
        ///
        /// It assumes the text has already been parsed and does not contain the comment markers anymore.
        ///
        /// \returns The root node of the AST.
        ast_root read_ast(const parser& p, const std::string& comment);

        /// An inline comment.
        class inline_comment
        {
        public:
            /// \effects Creates it giving it the metadata and brief and details section.
            /// \requires `data.is_inline()`.
            inline_comment(comment::metadata data, std::unique_ptr<markup::brief_section> brief,
                           std::unique_ptr<markup::details_section> details)
            : data_(std::move(data)), brief_(std::move(brief)), details_(std::move(details))
            {
            }

            /// \effects Creates it giving it the metadata and brief section.
            /// \requires `data.is_inline()`.
            inline_comment(comment::metadata data, std::unique_ptr<markup::brief_section> brief)
            : inline_comment(std::move(data), std::move(brief), nullptr)
            {
            }

            /// \returns The metadata.
            const comment::metadata& metadata() const noexcept
            {
                return data_;
            }

            /// \returns The brief section.
            const markup::brief_section& brief_section() const noexcept
            {
                return *brief_;
            }

            /// \returns The details section, if there is one.
            type_safe::optional_ref<const markup::details_section> details_section() const noexcept
            {
                return type_safe::opt_ref(details_.get());
            }

        private:
            comment::metadata                        data_;
            std::unique_ptr<markup::brief_section>   brief_;
            std::unique_ptr<markup::details_section> details_;
        };

        /// The translated CommonMark AST.
        ///
        /// All the information are extracted.
        class translated_ast
        {
            using section_iterator = markup::container_entity<markup::doc_section>::iterator;

            class section_range
            {
            public:
                section_range(const std::vector<std::unique_ptr<markup::doc_section>>& sections)
                : begin_(sections.begin()), end_(sections.end())
                {
                }

                std::size_t size() const noexcept
                {
                    return std::size_t(end_ - begin_);
                }

                section_iterator begin() const noexcept
                {
                    return begin_;
                }

                section_iterator end() const noexcept
                {
                    return end_;
                }

            private:
                std::vector<std::unique_ptr<markup::doc_section>>::const_iterator begin_, end_;
            };

        public:
            class builder;

            /// \returns A range-like object to the non-brief documentation sections.
            section_range sections() const noexcept
            {
                return section_range(sections_);
            }

            /// \returns A reference to the brief section, if there is one.
            type_safe::optional_ref<const markup::brief_section> brief_section() const noexcept
            {
                return type_safe::opt_ref(brief_.get());
            }

            /// \returns The metadata of the comment.
            const comment::metadata& metadata() const noexcept
            {
                return data_;
            }

            /// \returns A range-like object to the inline comments of the entity.
            /// \exclude return
            const std::vector<inline_comment>& inlines() const noexcept
            {
                return inlines_;
            }

        private:
            translated_ast() = default;

            comment::metadata                                 data_;
            std::vector<std::unique_ptr<markup::doc_section>> sections_;
            std::vector<inline_comment>                       inlines_;
            std::unique_ptr<markup::brief_section>            brief_;
        };

        class translated_ast::builder
        {
        public:
            builder() = default;

            /// \effects Adds a documentation section.
            /// \requires It must not be a brief section.
            void add_section(std::unique_ptr<markup::doc_section> sec)
            {
                result_.sections_.push_back(std::move(sec));
            }

            /// \effects Adds an inline comment.
            void add_inline(inline_comment c)
            {
                result_.inlines_.push_back(std::move(c));
            }

            /// \effects Sets the brief section.
            /// \returns Whether or not it was called the first time.
            bool brief(std::unique_ptr<markup::brief_section> brief)
            {
                auto set       = bool(result_.brief_);
                result_.brief_ = std::move(brief);
                return !set;
            }

            /// \returns A reference to the metadata.
            comment::metadata& metadata()
            {
                return result_.data_;
            }

            /// \returns The finished AST.
            translated_ast finish()
            {
                return std::move(result_);
            }

        private:
            translated_ast result_;
        };

        /// A translation error.
        class translation_error : public std::runtime_error
        {
        public:
            /// \effects Creates it given the line and column of the error,
            /// and a message.
            translation_error(unsigned line, unsigned column, std::string msg)
            : std::runtime_error(std::move(msg)), line_(line), column_(column)
            {
            }

            /// \returns The line of the comment where the error occurs.
            unsigned line() const noexcept
            {
                return line_;
            }

            /// \returns The column of the comment where the error occurs.
            unsigned column() const noexcept
            {
                return column_;
            }

        private:
            unsigned line_, column_;
        };

        /// Translates the CommonMark AST.
        /// \returns The translated AST.
        translated_ast translate_ast(const parser& p, const ast_root& root);
    }
} // namespace standardese::comment

#endif // STANDARDESE_COMMENT_PARSER_HPP_INCLUDED
