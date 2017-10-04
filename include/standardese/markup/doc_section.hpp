// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_DOC_SECTION_HPP_INCLUDED
#define STANDARDESE_MARKUP_DOC_SECTION_HPP_INCLUDED

#include <standardese/markup/block.hpp>
#include <standardese/markup/list.hpp>
#include <standardese/markup/paragraph.hpp>
#include <standardese/markup/phrasing.hpp>

namespace standardese
{
    namespace markup
    {
        /// The type of a documentation section.
        enum class section_type : unsigned
        {
            brief,
            details,

            // [structure.specifications]/3 sections
            requires,
            effects,
            synchronization,
            postconditions,
            returns,
            throws,
            complexity,
            remarks,
            error_conditions,
            notes,

            see,

            count,
            invalid = count
        };

        /// A special section in an entity documentation.
        class doc_section : public entity
        {
        public:
            /// \returns The type of section it is.
            /// It may return [*section_type::invalid]() if there is no directly associated type.
            section_type type() const noexcept
            {
                return do_get_section_type();
            }

        protected:
            doc_section() noexcept = default;

        private:
            virtual section_type do_get_section_type() const noexcept = 0;
        };

        /// The `\brief` section in an entity documentation.
        class brief_section final : public doc_section, public container_entity<phrasing_entity>
        {
        public:
            /// Builds a brief section.
            class builder : public container_builder<brief_section>
            {
            public:
                /// \effects Creates an empty brief section.
                builder() : container_builder(std::unique_ptr<brief_section>(new brief_section()))
                {
                }
            };

            /// \returns The unique id of the brief section.
            ///
            /// It is created from the parent id.
            block_id id() const;

        private:
            entity_kind do_get_kind() const noexcept override;

            section_type do_get_section_type() const noexcept
            {
                return section_type::brief;
            }

            void do_visit(detail::visitor_callback_t cb, void* mem) const override;

            std::unique_ptr<entity> do_clone() const override;

            brief_section() = default;
        };

        /// A `\details` section in an entity documentation.
        /// \notes There can be multiple `\details` in a documentation, but only one `\brief`.
        class details_section final : public doc_section, public container_entity<block_entity>
        {
        public:
            /// Builds a details section.
            class builder : public container_builder<details_section>
            {
            public:
                /// \effects Creates an empty details section.
                builder()
                : container_builder(std::unique_ptr<details_section>(new details_section()))
                {
                }
            };

        private:
            entity_kind do_get_kind() const noexcept override;

            section_type do_get_section_type() const noexcept override
            {
                return section_type::details;
            }

            void do_visit(detail::visitor_callback_t cb, void* mem) const override;

            std::unique_ptr<entity> do_clone() const override;

            details_section() = default;
        };

        /// A section like `\effects` that just contains some text.
        ///
        /// It cannot be used for `\brief` or `\details`,
        /// use [standardese::markup::brief_section]() or [standardese::markup::details_section]() for that.
        class inline_section final : public doc_section
        {
        public:
            /// Builds a section.
            class builder
            {
            public:
                /// \effects Creates an empty section of given type and using the given name.
                /// \notes `name` should not contain the trailing `:`.
                builder(section_type type, std::string name)
                : result_(new inline_section(type, std::move(name), nullptr))
                {
                }

                /// \effects Adds a child to the section.
                builder& add_child(std::unique_ptr<phrasing_entity> entity)
                {
                    paragraph_.add_child(std::move(entity));
                    return *this;
                }

                /// \returns The finished section.
                std::unique_ptr<inline_section> finish() noexcept
                {
                    result_->paragraph_ = paragraph_.finish();
                    result_->set_ownership(*result_->paragraph_);
                    return std::move(result_);
                }

            private:
                std::unique_ptr<inline_section> result_;
                paragraph::builder              paragraph_;
            };

            /// \returns The newly built inline section.
            /// It will use the children of paragraph for itself.
            static std::unique_ptr<inline_section> build(
                section_type type, std::string name, std::unique_ptr<markup::paragraph> paragraph)
            {
                return std::unique_ptr<inline_section>(
                    new inline_section(type, std::move(name), std::move(paragraph)));
            }

            /// \returns The name of the section.
            const std::string& name() const noexcept
            {
                return name_;
            }

            /// \returns An iterator to the first child.
            container_entity<phrasing_entity>::iterator begin() const noexcept
            {
                return paragraph_->begin();
            }

            /// \returns An iterator one past the last child.
            container_entity<phrasing_entity>::iterator end() const noexcept
            {
                return paragraph_->end();
            }

        private:
            entity_kind do_get_kind() const noexcept override;

            section_type do_get_section_type() const noexcept override
            {
                return type_;
            }

            void do_visit(detail::visitor_callback_t cb, void* mem) const override;

            std::unique_ptr<entity> do_clone() const override;

            inline_section(section_type type, std::string name,
                           std::unique_ptr<markup::paragraph> paragraph)
            : name_(std::move(name)), paragraph_(std::move(paragraph)), type_(type)
            {
            }

            std::string                        name_;
            std::unique_ptr<markup::paragraph> paragraph_;
            section_type                       type_;
        };

        /// A section containing a list.
        ///
        /// This can be a `\returns` with an entry for each return value, for example.
        class list_section final : public doc_section
        {
        public:
            /// \returns A newly built list section.
            static std::unique_ptr<list_section> build(section_type type, std::string name,
                                                       std::unique_ptr<unordered_list> list);

            /// \returns The name of the section.
            const std::string& name() const noexcept
            {
                return name_;
            }

            /// \returns The id of the section.
            const block_id& id() const noexcept
            {
                return list_->id();
            }

            using iterator = unordered_list::iterator;

            /// \returns An iterator to the first list item.
            iterator begin() const noexcept
            {
                return list_->begin();
            }

            /// \returns An iterator one past the last item.
            iterator end() const noexcept
            {
                return list_->end();
            }

        private:
            entity_kind do_get_kind() const noexcept override;

            section_type do_get_section_type() const noexcept override
            {
                return type_;
            }

            void do_visit(detail::visitor_callback_t cb, void* mem) const override;

            std::unique_ptr<entity> do_clone() const override;

            list_section(section_type type, std::string name, std::unique_ptr<unordered_list> list);

            std::string                     name_;
            std::unique_ptr<unordered_list> list_;
            section_type                    type_;
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_DOC_SECTION_HPP_INCLUDED
