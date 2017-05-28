// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_PARSER_HPP_INCLUDED
#define STANDARDESE_COMMENT_PARSER_HPP_INCLUDED

#include <vector>

#include <standardese/markup/doc_section.hpp>

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
        /// This is just a RAII wrapper over the `cmark_parser`.
        class parser
        {
        public:
            /// \effects Creates a new parser.
            parser();

            ~parser() noexcept;

            parser(const parser&) = delete;
            parser& operator=(const parser&) = delete;

            /// \returns The parser.
            cmark_parser* get() const noexcept
            {
                return parser_;
            }

        private:
            cmark_parser* parser_;
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

        /// The translated CommonMark AST.
        ///
        /// All the information are extracted.
        struct translated_ast
        {
            /// The sections in the documentation text.
            std::vector<std::unique_ptr<markup::doc_section>> sections;
        };

        /// Translates the CommonMark AST.
        /// \returns The translated AST.
        translated_ast translate_ast(const ast_root& root);
    }
} // namespace standardese::comment

#endif // STANDARDESE_COMMENT_PARSER_HPP_INCLUDED
