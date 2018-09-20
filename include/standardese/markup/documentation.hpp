// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_DOCUMENTATION_HPP_INCLUDED
#define STANDARDESE_MARKUP_DOCUMENTATION_HPP_INCLUDED

#include <type_safe/optional_ref.hpp>

#include <vector>

#include <standardese/markup/block.hpp>
#include <standardese/markup/code_block.hpp>
#include <standardese/markup/doc_section.hpp>
#include <standardese/markup/entity.hpp>
#include <standardese/markup/heading.hpp>

namespace cppast
{
class cpp_entity;
class cpp_file;
} // namespace cppast

namespace standardese
{
namespace markup
{
    /// The heading in a documentation.
    class documentation_header
    {
    public:
        /// \effects Creates it passing it heading and module.
        documentation_header(std::unique_ptr<markup::heading> h,
                             type_safe::optional<std::string> module = type_safe::nullopt)
        : module_(std::move(module)), heading_(std::move(h))
        {}

        documentation_header clone() const;

        /// \returns A reference to the heading of the documentation.
        /// \group heading
        const markup::heading& heading() const noexcept
        {
            return *heading_;
        }

        /// \group heading
        markup::heading& heading() noexcept
        {
            return *heading_;
        }

        /// \returns The module of the entity, if any.
        const type_safe::optional<std::string>& module() const noexcept
        {
            return module_;
        }

    private:
        type_safe::optional<std::string> module_;
        std::unique_ptr<markup::heading> heading_;
    };

    /// The documentation of an entity.
    class documentation_entity : public block_entity
    {
    public:
        /// \returns The header of a documentation, if any.
        const type_safe::optional<documentation_header>& header() const noexcept
        {
            return header_;
        }

        /// \returns A reference to the synopsis of the entity, if there is any.
        type_safe::optional_ref<const code_block> synopsis() const noexcept
        {
            return type_safe::opt_ref(synopsis_.get());
        }

        /// \returns A reference to the `\brief` section of the documentation,
        /// if there is any.
        type_safe::optional_ref<const markup::brief_section> brief_section() const noexcept;

        /// \returns A reference to the `\details` section of the documentation,
        /// if there is any.
        type_safe::optional_ref<const markup::details_section> details_section() const noexcept;

        /// \returns An iteratable object iterating over all the sections (including brief and
        /// details), in the order they were given.
        detail::vector_ptr_range<doc_section> doc_sections() const noexcept
        {
            return {sections_.begin(), sections_.end()};
        }

    protected:
        documentation_entity(block_id id, type_safe::optional<documentation_header> h,
                             std::unique_ptr<code_block> synopsis) // may be nullptr
        : block_entity(std::move(id)), header_(std::move(h)), synopsis_(std::move(synopsis))
        {
            if (synopsis_)
                set_ownership(*synopsis_);
            if (header_)
                set_ownership(header_.value().heading());
        }

        template <class BaseBuilder>
        class documentation_builder : public BaseBuilder
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

            /// \returns The id of the documentation.
            const block_id& id() const noexcept
            {
                return this->peek().id();
            }

            /// \returns Whether or not it has any documentation sections.
            bool has_documentation() const noexcept
            {
                return !this->peek().sections_.empty();
            }

        protected:
            using BaseBuilder::BaseBuilder;

            void add_section_impl(std::unique_ptr<doc_section> section)
            {
                detail::parent_updater::set(*section, type_safe::ref(this->peek()));
                this->peek().sections_.push_back(std::move(section));
            }

            friend class entity_documentation;
            friend class file_documentation;
            friend class namespace_documentation;
            friend class module_documentation;
        };

    private:
        std::vector<std::unique_ptr<doc_section>> sections_;
        type_safe::optional<documentation_header> header_;
        std::unique_ptr<code_block>               synopsis_; // may be nullptr
    };

    /// A container containing the documentation of a single entity.
    ///
    /// It can optionally have a [standardese::markup::heading]() which will be rendered as well.
    /// If it has a heading, it will also render a [standardese::markup::thematic_break]() at the
    /// end. \notes This does not represent the documentation of a file, use
    /// [standardese::markup::file_documentation]() for that.
    class entity_documentation final : public documentation_entity,
                                       public container_entity<entity_documentation>
    {
    public:
        /// Builds the documentation of an entity.
        class builder : public documentation_builder<container_builder<entity_documentation>>
        {
        public:
            /// \effects Creates it giving the id, header and synopsis.
            /// \requires The user data of the entity must either be `nullptr` or the corresponding
            /// [standardese::doc_entity]().
            builder(type_safe::object_ref<const cppast::cpp_entity> entity, block_id id,
                    type_safe::optional<documentation_header> h,
                    std::unique_ptr<code_block>               synopsis)
            : documentation_builder(std::unique_ptr<entity_documentation>(
                  new entity_documentation(entity, std::move(id), std::move(h),
                                           std::move(synopsis))))
            {}
        };

        const cppast::cpp_entity& entity() const noexcept
        {
            return *entity_;
        }

    private:
        entity_documentation(type_safe::object_ref<const cppast::cpp_entity> entity, block_id id,
                             type_safe::optional<documentation_header> h,
                             std::unique_ptr<code_block>               synopsis)
        : documentation_entity(std::move(id), std::move(h), std::move(synopsis)), entity_(entity)
        {}

        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<markup::entity> do_clone() const override;

        type_safe::object_ref<const cppast::cpp_entity> entity_;
    };

    /// The documentation of a file.
    ///
    /// \notes This does not represent a stand-alone file, use a
    /// [standardese::markup::document_entity]() for that.
    class file_documentation final : public documentation_entity,
                                     public container_entity<entity_documentation>
    {
    public:
        /// Builds the documentation of a file.
        class builder : public documentation_builder<container_builder<file_documentation>>
        {
        public:
            /// \effects Creates it giving the id, header and synopsis.
            /// \requires The user data of the file must either be `nullptr` or the corresponding
            /// [standardese::doc_entity]().
            builder(type_safe::object_ref<const cppast::cpp_file> f, block_id id,
                    type_safe::optional<documentation_header> h,
                    std::unique_ptr<code_block>               synopsis)
            : documentation_builder(std::unique_ptr<file_documentation>(
                  new file_documentation(f, std::move(id), std::move(h), std::move(synopsis))))
            {}
        };

        const cppast::cpp_file& file() const noexcept
        {
            return *file_;
        }

    private:
        file_documentation(type_safe::object_ref<const cppast::cpp_file> f, block_id id,
                           type_safe::optional<documentation_header> h,
                           std::unique_ptr<code_block>               synopsis)
        : documentation_entity(std::move(id), std::move(h), std::move(synopsis)), file_(f)
        {}

        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;

        type_safe::object_ref<const cppast::cpp_file> file_;
    };
} // namespace markup
} // namespace standardese

#endif // STANDARDESE_MARKUP_DOCUMENTATION_HPP_INCLUDED
