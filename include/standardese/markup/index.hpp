// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_INDEX_HPP_INCLUDED
#define STANDARDESE_MARKUP_INDEX_HPP_INCLUDED

#include <standardese/markup/documentation.hpp>
#include <standardese/markup/heading.hpp>
#include <standardese/markup/list.hpp>

namespace cppast
{
class cpp_namespace;
} // namespace cppast

namespace standardese
{
namespace markup
{
    /// Base class for all kinds of documentation indices.
    class index_entity : public block_entity
    {
    public:
        /// \returns The heading of the index.
        const markup::heading& heading() const noexcept
        {
            return *heading_;
        }

    protected:
        index_entity(block_id id, std::unique_ptr<markup::heading> h)
        : block_entity(std::move(id)), heading_(std::move(h))
        {}

    private:
        std::unique_ptr<markup::heading> heading_;
    };

    /// A list item that is an entity of the [standardese::entity_index]().
    /// \notes This can also be used for the file index and namespace index.
    class entity_index_item final : public list_item_base
    {
    public:
        /// \returns A newly built entity index item.
        static std::unique_ptr<entity_index_item> build(block_id id, std::unique_ptr<term> entity,
                                                        std::unique_ptr<description> brief
                                                        = nullptr)
        {
            return std::unique_ptr<entity_index_item>(
                new entity_index_item(std::move(id), std::move(entity), std::move(brief)));
        }

        /// \returns The entity name.
        const term& entity() const noexcept
        {
            return *entity_;
        }

        /// \returns The brief description of the entity, if it has any.
        type_safe::optional_ref<const description> brief() const noexcept
        {
            return type_safe::opt_ref(brief_.get());
        }

    private:
        entity_index_item(block_id id, std::unique_ptr<term> entity,
                          std::unique_ptr<description> brief)
        : list_item_base(std::move(id)), entity_(std::move(entity)), brief_(std::move(brief))
        {}

        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<markup::entity> do_clone() const override;

        std::unique_ptr<term>        entity_;
        std::unique_ptr<description> brief_;
    };

    /// The index of all files.
    /// \notes This is created by the [standardese::file_index]().
    class file_index final : public index_entity, public container_entity<entity_index_item>
    {
    public:
        /// Builds the file index.
        class builder : public container_builder<file_index>
        {
        public:
            /// \effects Creates it given the heading.
            builder(std::unique_ptr<markup::heading> h)
            : container_builder(std::unique_ptr<file_index>(new file_index(std::move(h))))
            {}
        };

    private:
        file_index(std::unique_ptr<markup::heading> h)
        : index_entity(block_id("file-index"), std::move(h))
        {}

        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;
    };

    /// The documentation of a namespace.
    ///
    /// It is meant to be used in the entity index.
    class namespace_documentation final : public documentation_entity,
                                          public container_entity<block_entity>
    {
    public:
        /// Builds the documentation of a namespace.
        class builder : public documentation_builder<container_builder<namespace_documentation>>
        {
        public:
            /// \effects Creates it giving the id and header.
            /// \requires The user data of the namespace must either be `nullptr` or the
            /// corresponding [standardese::doc_entity]().
            builder(type_safe::object_ref<const cppast::cpp_namespace> ns, block_id id,
                    type_safe::optional<documentation_header> h)
            : documentation_builder(std::unique_ptr<namespace_documentation>(
                  new namespace_documentation(ns, std::move(id), std::move(h))))
            {}

            builder& add_child(std::unique_ptr<entity_index_item> entity)
            {
                container_builder::add_child(std::move(entity));
                return *this;
            }

            builder& add_child(std::unique_ptr<namespace_documentation> entity)
            {
                container_builder::add_child(std::move(entity));
                return *this;
            }

        private:
            using container_builder::add_child;
        };

        const cppast::cpp_namespace& namespace_() const noexcept
        {
            return *ns_;
        }

    private:
        namespace_documentation(type_safe::object_ref<const cppast::cpp_namespace> ns, block_id id,
                                type_safe::optional<documentation_header> h)
        : documentation_entity(std::move(id), std::move(h), nullptr), ns_(ns)
        {}

        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;

        type_safe::object_ref<const cppast::cpp_namespace> ns_;
    };

    /// The index of all entities.
    /// \notes This is created by [standardese::entity_index]().
    class entity_index final : public index_entity, public container_entity<block_entity>
    {
    public:
        /// Builds the entity index.
        class builder : public container_builder<entity_index>
        {
        public:
            /// \effects Creates it given the heading.
            builder(std::unique_ptr<markup::heading> h)
            : container_builder(std::unique_ptr<entity_index>(new entity_index(std::move(h))))
            {}

            builder& add_child(std::unique_ptr<entity_index_item> item)
            {
                container_builder::add_child(std::move(item));
                return *this;
            }

            builder& add_child(std::unique_ptr<namespace_documentation> ns)
            {
                container_builder::add_child(std::move(ns));
                return *this;
            }

        private:
            using container_builder::add_child;
        };

    private:
        entity_index(std::unique_ptr<markup::heading> h)
        : index_entity(block_id("entity-index"), std::move(h))
        {}

        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;
    };

    /// The documentation of a module.
    ///
    /// It is meant to be used in the module index.
    class module_documentation final : public documentation_entity,
                                       public container_entity<entity_index_item>
    {
    public:
        /// Builds the documentation of a module.
        class builder : public documentation_builder<container_builder<module_documentation>>
        {
        public:
            /// \effects Creates it giving the id and header.
            builder(block_id id, type_safe::optional<documentation_header> h)
            : documentation_builder(std::unique_ptr<module_documentation>(
                  new module_documentation(std::move(id), std::move(h), nullptr)))
            {}
        };

    private:
        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;

        using documentation_entity::documentation_entity;
    };

    /// The index of all module.
    /// \notes This is created by the [standardese::module_index]().
    class module_index final : public index_entity, public container_entity<module_documentation>
    {
    public:
        /// Builds the file index.
        class builder : public container_builder<module_index>
        {
        public:
            /// \effects Creates it given the heading.
            builder(std::unique_ptr<markup::heading> h)
            : container_builder(std::unique_ptr<module_index>(new module_index(std::move(h))))
            {}
        };

    private:
        module_index(std::unique_ptr<markup::heading> h)
        : index_entity(block_id("module-index"), std::move(h))
        {}

        entity_kind do_get_kind() const noexcept override;

        void do_visit(detail::visitor_callback_t cb, void* mem) const override;

        std::unique_ptr<entity> do_clone() const override;
    };
} // namespace markup
} // namespace standardese

#endif // STANDARDESE_MARKUP_INDEX_HPP_INCLUDED
