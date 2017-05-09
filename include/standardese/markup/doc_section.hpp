// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_DOC_SECTION_HPP_INCLUDED
#define STANDARDESE_MARKUP_DOC_SECTION_HPP_INCLUDED

#include <standardese/markup/block.hpp>
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

            brief_section() = default;
        };

        /// The `\details` section in an entity documentation.
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

            /// \returns The unique id of the details section.
            ///
            /// It is created from the parent id.
            block_id id() const;

        private:
            entity_kind do_get_kind() const noexcept override;

            section_type do_get_section_type() const noexcept override
            {
                return section_type::details;
            }

            details_section() = default;
        };

        /// A section like `\effects` that just contains some text.
        ///
        /// It cannot be used for `\brief` or `\details`,
        /// use [standardese::markup::brief_section]() or [standardese::markup::details_section]() for that.
        class inline_section final : public doc_section, public container_entity<phrasing_entity>
        {
        public:
            /// Builds a section.
            class builder : public container_builder<inline_section>
            {
            public:
                /// \effects Creates an empty section of given type and using the given name.
                /// \notes `name` should not contain the trailing `:`.
                builder(section_type type, std::string name)
                : container_builder(
                      std::unique_ptr<inline_section>(new inline_section(type, std::move(name))))
                {
                }
            };

            /// \returns The name of the section.
            const std::string& name() const noexcept
            {
                return name_;
            }

        private:
            entity_kind do_get_kind() const noexcept override;

            section_type do_get_section_type() const noexcept override
            {
                return type_;
            }

            inline_section(section_type type, std::string name)
            : name_(std::move(name)), type_(type)
            {
            }

            std::string  name_;
            section_type type_;
        };

        class unordered_list;

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

            /// \returns The list of the section.
            const unordered_list& list() const noexcept
            {
                return *list_;
            }

        private:
            entity_kind do_get_kind() const noexcept override;

            section_type do_get_section_type() const noexcept override
            {
                return type_;
            }

            list_section(section_type type, std::string name, std::unique_ptr<unordered_list> list);

            std::string                     name_;
            std::unique_ptr<unordered_list> list_;
            section_type                    type_;
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_DOC_SECTION_HPP_INCLUDED
