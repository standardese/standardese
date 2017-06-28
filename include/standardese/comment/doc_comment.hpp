// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_DOC_COMMENT_HPP_INCLUDED
#define STANDARDESE_COMMENT_DOC_COMMENT_HPP_INCLUDED

#include <standardese/comment/metadata.hpp>
#include <standardese/markup/documentation.hpp>
#include <standardese/markup/doc_section.hpp>

namespace standardese
{
    namespace comment
    {
        /// The comment associated with an entity.
        class doc_comment
        {
            using section_iterator = markup::container_entity<markup::doc_section>::iterator;

            class section_range
            {
            public:
                section_range(const std::vector<std::unique_ptr<markup::doc_section>>& sections)
                : begin_(sections.begin()), end_(sections.end())
                {
                }

                bool empty() const noexcept
                {
                    return begin_ == end_;
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
            /// \effects Creates it giving the metadata and all the sections.
            /// \requires Sections
            doc_comment(comment::metadata metadata, std::unique_ptr<markup::brief_section> brief,
                        std::vector<std::unique_ptr<markup::doc_section>> sections)
            : metadata_(std::move(metadata)),
              sections_(std::move(sections)),
              brief_(std::move(brief))
            {
            }

            /// \returns The metadata of the comment.
            const comment::metadata& metadata() const noexcept
            {
                return metadata_;
            }

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

        private:
            comment::metadata                                 metadata_;
            std::vector<std::unique_ptr<markup::doc_section>> sections_;
            std::unique_ptr<markup::brief_section>            brief_;
        };

        /// \effects Adds a copy of the sections to the documentation builder.
        /// \group set_sections
        void set_sections(markup::entity_documentation::builder& builder,
                          const doc_comment&                     comment);

        /// \group set_sections
        void set_sections(markup::file_documentation::builder& builder, const doc_comment& comment);
    }
} // namespace standardese::comment

#endif // STANDARDESE_COMMENT_DOC_COMMENT_HPP_INCLUDED
