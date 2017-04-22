// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_DOCUMENT_HPP_INCLUDED
#define STANDARDESE_MARKUP_DOCUMENT_HPP_INCLUDED

#include <utility>

#include <standardese/markup/block.hpp>
#include <standardese/markup/entity.hpp>

namespace standardese
{
    namespace markup
    {
        /// Base class for entities representing a stand-alone document.
        ///
        /// Those are the root nodes of the markup AST.
        class document_entity : public entity, public entity_container<block_entity>
        {
        public:
            /// \returns The title of the document.
            const std::string& title() const noexcept
            {
                return title_;
            }

            /// \returns The output file name of the document, without extension.
            const std::string& output_name() const noexcept
            {
                return output_name_;
            }

            /// \returns Whether or not this document has an extension override.
            /// \notes This happens if an output template is used.
            bool has_extension_override() const noexcept
            {
                return !extension_.empty();
            }

            /// \returns The extension override, or an empty string, if it has none.
            const std::string& extension_override() const noexcept
            {
                return extension_;
            }

        protected:
            document_entity(std::string title, std::string output_name, std::string extension = "")
            : title_(std::move(title)),
              output_name_(std::move(output_name)),
              extension_(std::move(extension))
            {
            }

        private:
            std::string title_;
            std::string output_name_, extension_;
        };

        /// A document which is the main output page of the documentation.
        class main_document final : public document_entity
        {
        public:
            /// Builds the main document.
            class builder : public container_builder<main_document>
            {
            public:
                /// \effects Creates an empty document with given title and output file name.
                builder(std::string title, std::string output_name)
                : container_builder(std::unique_ptr<main_document>(
                      new main_document(std::move(title), std::move(output_name))))
                {
                }
            };

        private:
            void do_append_html(std::string& result) const override;

            main_document(std::string title, std::string output_name)
            : document_entity(std::move(title), std::move(output_name))
            {
            }
        };

        void generate_html(const main_document& document);

        /// A document which is not the main output page of the documentation.
        class sub_document final : public document_entity
        {
        public:
            /// Builds the sub document.
            class builder : public container_builder<sub_document>
            {
            public:
                /// \effects Creates an empty document with given title and output file name.
                builder(std::string title, std::string output_name)
                : container_builder(std::unique_ptr<sub_document>(
                      new sub_document(std::move(title), std::move(output_name))))
                {
                }
            };

        private:
            void do_append_html(std::string& result) const override;

            sub_document(std::string title, std::string output_name)
            : document_entity(std::move(title), std::move(output_name))
            {
            }
        };

        void generate_html(const sub_document& document);

        class template_document final : public document_entity
        {
        public:
            /// Builds a template document.
            class builder : public container_builder<template_document>
            {
            public:
                /// \effects Creates it given the title and the file name of the template file.
                /// If the file name contains an extension, it will be treated as an extension override.
                /// Else the extension is determined by the output format.
                /// \notes An extension is the substring after the last `.` character.
                builder(std::string title, std::string file_name)
                : container_builder(std::unique_ptr<template_document>(
                      new template_document(std::move(title), std::move(file_name))))
                {
                }
            };

        private:
            void do_append_html(std::string& result) const override;

            template_document(std::string title, std::string file_name);

            template_document(std::string title, std::pair<std::string, std::string> file_name)
            : document_entity(std::move(title), std::move(file_name.first),
                              std::move(file_name.second))
            {
            }
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_DOCUMENT_HPP_INCLUDED
