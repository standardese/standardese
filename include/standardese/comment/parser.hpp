// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_PARSER_HPP_INCLUDED
#define STANDARDESE_COMMENT_PARSER_HPP_INCLUDED

#include <stdexcept>
#include <vector>

#include <type_safe/optional.hpp>

#include <standardese/markup/doc_section.hpp>

#include <standardese/comment/config.hpp>
#include <standardese/markup/entity_kind.hpp>

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

        /// The exclude mode of an entity.
        enum class exclude_mode
        {
            entity,      //< The entire entity is excluded.
            return_type, //< The return type of a function is excluded.
            target       //< The target of an alias is excluded.
        };

        /// The group of an entity.
        class member_group
        {
        public:
            /// \effects Creates a group given the name,
            /// optional heading and whether or not the name is also an output section.
            member_group(std::string name, type_safe::optional<std::string> heading,
                         bool is_section)
            : heading_(std::move(heading)), name_(std::move(name)), is_section_(is_section)
            {
            }

            /// \returns The name of the group.
            const std::string& name() const noexcept
            {
                return name_;
            }

            /// \returns The heading of the group, if there is one.
            const type_safe::optional<std::string>& heading() const noexcept
            {
                return heading_;
            }

            /// \returns The output section of the group, if there is one.
            type_safe::optional_ref<const std::string> output_section() const noexcept
            {
                return is_section_ ? type_safe::optional_ref<const std::string>(name_) :
                                     type_safe::nullopt;
            }

        private:
            type_safe::optional<std::string> heading_;
            std::string                      name_;
            bool                             is_section_;
        };

        inline bool operator==(const member_group& a, const member_group& b)
        {
            return a.name() == b.name();
        }

        inline bool operator!=(const member_group& a, const member_group& b)
        {
            return !(a == b);
        }

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

            /// \returns The exclude mode of the entity, if there is one.
            const type_safe::optional<exclude_mode>& exclude() const noexcept
            {
                return exclude_;
            }

            /// \returns The unique name override, if there is one.
            const type_safe::optional<std::string>& unique_name() const noexcept
            {
                return unique_name_;
            }

            /// \returns The synopsis override, if there is one.
            const type_safe::optional<std::string>& synopsis() const noexcept
            {
                return synopsis_;
            }

            /// \returns The group, if it is in one.
            const type_safe::optional<member_group>& group() const noexcept
            {
                return group_;
            }

            /// \returns The name of the module, if there is one.
            const type_safe::optional<std::string>& module() const noexcept
            {
                return module_;
            }

            /// \returns The output section, if there is one.
            const type_safe::optional<std::string>& output_section() const noexcept
            {
                return section_;
            }

        private:
            translated_ast() = default;

            type_safe::optional<member_group> group_;
            type_safe::optional<std::string>  unique_name_, synopsis_, module_, section_;

            std::vector<std::unique_ptr<markup::doc_section>> sections_;
            std::unique_ptr<markup::brief_section>            brief_;

            type_safe::optional<exclude_mode> exclude_;
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

            /// \effects Sets the brief section.
            /// \returns Whether or not it was called the first time.
            bool brief(std::unique_ptr<markup::brief_section> brief)
            {
                auto set       = bool(result_.brief_);
                result_.brief_ = std::move(brief);
                return !set;
            }

            /// \effects Sets the exclude mode.
            /// \returns Whether or not it was called the first time.
            bool exclude(exclude_mode mode)
            {
                auto set         = bool(result_.exclude_);
                result_.exclude_ = mode;
                return !set;
            }

            /// \effects Sets the unique name override.
            /// \returns Whether or not it was called the first time.
            bool unique_name(std::string name)
            {
                auto set             = bool(result_.unique_name_);
                result_.unique_name_ = std::move(name);
                return !set;
            }

            /// \effects Sets the synopsis override.
            /// \returns Whether or not it was called the first time.
            bool synopsis(std::string name)
            {
                auto set          = bool(result_.synopsis_);
                result_.synopsis_ = std::move(name);
                return !set;
            }

            /// \effects Sets the group.
            /// \returns Whether or not it was called the first time.
            bool group(member_group g)
            {
                auto set       = bool(result_.group_);
                result_.group_ = std::move(g);
                return !set;
            }

            /// \effects Sets the name of the module.
            /// \returns Whether or not it was called the first time.
            bool module(std::string module)
            {
                auto set        = bool(result_.module_);
                result_.module_ = std::move(module);
                return !set;
            }

            /// \effects Sets the output section.
            /// \returns Whether or not it was called the first time.
            bool output_section(std::string section)
            {
                auto set         = bool(result_.section_);
                result_.section_ = std::move(section);
                return !set;
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
