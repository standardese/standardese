// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_DOCUMENTATION_HPP_INCLUDED
#define STANDARDESE_MARKUP_DOCUMENTATION_HPP_INCLUDED

#include <type_safe/optional_ref.hpp>

#include <vector>

#include <standardese/markup/block.hpp>
#include <standardese/markup/doc_section.hpp>
#include <standardese/markup/entity.hpp>

namespace standardese
{
    namespace markup
    {
        class heading;
        class code_block;

        /// The documentation of an entity.
        class documentation : public block_entity, public container_entity<documentation>
        {
        public:
            ~documentation() noexcept override;

            /// \returns A reference to the heading of the documentation.
            const markup::heading& heading() const noexcept
            {
                return *heading_;
            }

            /// \returns The module of the entity, if any.
            const type_safe::optional<std::string>& module() const noexcept
            {
                return module_;
            }

            /// \returns A reference to the synopsis of the entity.
            const code_block& synopsis() const noexcept
            {
                return *synopsis_;
            }

            /// \returns A reference to the `\brief` section of the documentation,
            /// if there is any.
            type_safe::optional_ref<const markup::brief_section> brief_section() const noexcept;

            /// \returns A reference to the `\details` section of the documentation,
            /// if there is any.
            type_safe::optional_ref<const markup::details_section> details_section() const noexcept;

            detail::vector_ptr_range<doc_section> doc_sections() const noexcept
            {
                return {sections_.begin(), sections_.end()};
            }

        protected:
            documentation(block_id id, std::unique_ptr<markup::heading> heading,
                          std::unique_ptr<code_block>      synopsis,
                          type_safe::optional<std::string> module);

            template <typename T>
            class documentation_builder : public container_builder<T>
            {
            public:
                /// \effects Adds a brief section.
                /// \requires This function must be called at most once.
                documentation_builder& add_brief(std::unique_ptr<markup::brief_section> section)
                {
                    add_section_impl(std::move(section));
                    return *this;
                }

                /// \effects Adds a details section.
                /// \requires This function must be called at most once.
                documentation_builder& add_details(std::unique_ptr<markup::details_section> section)
                {
                    add_section_impl(std::move(section));
                    return *this;
                }

                /// \effects Adds an inline section.
                documentation_builder& add_section(std::unique_ptr<inline_section> section)
                {
                    add_section_impl(std::move(section));
                    return *this;
                }

                /// \effects Adds a list section.
                documentation_builder& add_section(std::unique_ptr<list_section> section)
                {
                    add_section_impl(std::move(section));
                    return *this;
                }

            protected:
                using container_builder<T>::container_builder;

            private:
                void add_section_impl(std::unique_ptr<doc_section> section)
                {
                    detail::parent_updater::set(*section, type_safe::ref(this->peek()));
                    this->peek().sections_.push_back(std::move(section));
                }
            };

        private:
            std::vector<std::unique_ptr<doc_section>> sections_;
            type_safe::optional<std::string>          module_;
            std::unique_ptr<markup::heading>          heading_;
            std::unique_ptr<code_block>               synopsis_;
        };

        /// The documentation of a file.
        ///
        /// \notes This does not represent a stand-alone file, use a [standardese::markup::document_entity]() for that.
        class file_documentation final : public documentation
        {
        public:
            /// Builds the documentation of a file.
            class builder : public documentation_builder<file_documentation>
            {
            public:
                /// \effects Creates it giving the id, heading and synopsis and module.
                builder(block_id id, std::unique_ptr<markup::heading> h,
                        std::unique_ptr<code_block>      synopsis,
                        type_safe::optional<std::string> module = type_safe::nullopt)
                : documentation_builder(std::unique_ptr<file_documentation>(
                      new file_documentation(std::move(id), std::move(h), std::move(synopsis),
                                             std::move(module))))
                {
                }
            };

        private:
            entity_kind do_get_kind() const noexcept override;

            using documentation::documentation;
        };

        /// A container containing the documentation of a single entity.
        ///
        /// It can optionally have a [standardese::markup::heading]() which will be rendered as well.
        /// If it has a heading, it will also render a [standardese::markup::thematic_break]() at the end.
        /// \notes This does not represent the documentation of a file, use [standardese::markup::file_documentation]() for that.
        class entity_documentation final : public documentation
        {
        public:
            /// Builds the documentation of an entity.
            class builder : public documentation_builder<entity_documentation>
            {
            public:
                /// \effects Creates it giving the id, heading and synopsis and optional module.
                builder(block_id id, std::unique_ptr<markup::heading> h,
                        std::unique_ptr<code_block>      synopsis,
                        type_safe::optional<std::string> module = type_safe::nullopt)
                : documentation_builder(std::unique_ptr<entity_documentation>(
                      new entity_documentation(std::move(id), std::move(h), std::move(synopsis),
                                               std::move(module))))
                {
                }
            };

        private:
            entity_kind do_get_kind() const noexcept override;

            using documentation::documentation;
        };
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_DOCUMENTATION_HPP_INCLUDED
