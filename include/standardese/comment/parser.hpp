// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_PARSER_HPP_INCLUDED
#define STANDARDESE_COMMENT_PARSER_HPP_INCLUDED

#include <stdexcept>
#include <vector>

#include <type_safe/optional.hpp>

#include <standardese/markup/doc_section.hpp>
#include <standardese/markup/documentation.hpp>

#include <standardese/comment/config.hpp>
#include <standardese/comment/doc_comment.hpp>
#include <standardese/comment/matching_entity.hpp>
#include <standardese/comment/metadata.hpp>

extern "C"
{
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

    /// An unmatched documentation comment.
    ///
    /// That is, a comment not yet associated with an entity
    /// and still containing the matching entity information.
    struct unmatched_doc_comment
    {
        doc_comment     comment;
        matching_entity entity;

        unmatched_doc_comment(matching_entity entity, doc_comment comment)
        : comment(std::move(comment)), entity(std::move(entity))
        {}
    };

    /// The result of parsing.
    struct parse_result
    {
        type_safe::optional<doc_comment>
                                           comment; //< The comment or null if there were only inline entities.
        matching_entity                    entity;  //< The matching entity.
        std::vector<unmatched_doc_comment> inlines; //< The inline entities.
    };

    /// A parse error.
    class parse_error : public std::runtime_error
    {
    public:
        /// \effects Creates it given the line and column of the error,
        /// and a message.
        parse_error(unsigned line, unsigned column, std::string msg)
        : std::runtime_error(std::move(msg)), line_(line), column_(column)
        {}

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

    /// Parses the comment.
    /// \returns The parsed comment.
    /// \throws [standardese::comment::parse_error]() if an error occurred.
    parse_result parse(const parser& p, const std::string& comment, bool has_matching_entity);
} // namespace comment
} // namespace standardese

#endif // STANDARDESE_COMMENT_PARSER_HPP_INCLUDED
