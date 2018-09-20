// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_INDEX_HPP_INCLUDED
#define STANDARDESE_INDEX_HPP_INCLUDED

#include <memory>
#include <mutex>
#include <vector>

#include <type_safe/reference.hpp>
#include <type_safe/variant.hpp>

#include <standardese/markup/index.hpp>

namespace cppast
{
class cpp_entity;
class cpp_file;
class cpp_namespace;
} // namespace cppast

namespace standardese
{
namespace markup
{
    class document_entity;
} // namespace markup

/// An index of all the namespace level entities.
///
/// This should only include entities that are direct or indirect children of namespaces,
/// or a the global scope.
/// Not nested classes etc.
class entity_index
{
public:
    /// \effects Registers an entity and its documentation.
    /// Duplicate registration has no effect.
    /// \requires The entity must not be a file or namespace and must be at namespace or global
    /// scope. The user data of the entity must be `nullptr` or the corresponding
    /// [standardese::doc_entity]. \notes This function is thread safe.
    void register_entity(std::string link_name, const cppast::cpp_entity& entity,
                         type_safe::optional_ref<const markup::brief_section> brief) const;

    /// \effects Registers a namespace and its (incomplete) documentation.
    /// The user data of the namespace must be `nullptr` or the corresponding
    /// [standardese::doc_entity]. Duplicate registration will merge documentation. \notes This
    /// function is thread safe.
    void register_namespace(const cppast::cpp_namespace&             ns,
                            markup::namespace_documentation::builder doc) const;

    /// How the entities are ordered.
    enum order
    {
        namespace_inline_sorted, //< Namespaces inline with other entities, alphabetically sorted.
        namespace_external,      //< Namespaces separate from other entities, alphabetically sorted.
    };

    /// \returns The markup containing the index of all entities registered so far.
    /// \requires This function must only be called once.
    /// \notes This function is thread safe.
    std::unique_ptr<markup::entity_index> generate(order o) const;

private:
    struct entity
    {
        std::string name, scope;
        type_safe::variant<std::unique_ptr<markup::entity_index_item>,
                           markup::namespace_documentation::builder>
            doc;

        entity(std::unique_ptr<markup::entity_index_item> doc, std::string name, std::string scope)
        : name(std::move(name)), scope(std::move(scope)), doc(std::move(doc))
        {}

        entity(markup::namespace_documentation::builder doc, std::string name, std::string scope)
        : name(std::move(name)), scope(std::move(scope)), doc(std::move(doc))
        {}
    };

    void insert(entity e) const;

    mutable std::mutex          mutex_;
    mutable std::vector<entity> entities_; // sorted by scope, then name
};

/// Registers all entities that needs registration.
/// \effects It will visit all namespace-level entities of the file and registers them at index.
/// Namespaces itself are also registered using the user data to retrieve the documentation from the
/// [standardese::doc_entity].
void register_index_entities(const entity_index& index, const cppast::cpp_file& file);

/// An index of all the files.
class file_index
{
public:
    /// \effects Registers the given file and its documentation.
    /// Duplicate registration has no effect.
    /// \notes This function is thread safe.
    void register_file(std::string link_name, std::string file_name,
                       type_safe::optional_ref<const markup::brief_section> brief) const;

    /// \returns The markup containing the index of all files registered so far.
    /// \requires This function must only be called once.
    /// \notes This function is thread safe.
    std::unique_ptr<markup::file_index> generate() const;

private:
    struct file
    {
        std::string                                name;
        std::unique_ptr<markup::entity_index_item> doc;

        file(std::string name, std::unique_ptr<markup::entity_index_item> doc)
        : name(std::move(name)), doc(std::move(doc))
        {}
    };

    mutable std::mutex        mutex_;
    mutable std::vector<file> files_;
};

/// An index of all the modules.
class module_index
{
public:
    /// \effects Registers a module passing its (incomplete) documentation.
    /// Duplicate registration has no effect.
    /// \notes This function is thread safe.
    void register_module(markup::module_documentation::builder doc) const;

    /// \effects Registers an entity for the given module.
    /// \returns Whether or not there was a module already.
    /// If `false`, this function had no effect.
    /// \notes This function is thread safe.
    bool register_entity(std::string module, std::string link_name,
                         const cppast::cpp_entity&                            entity,
                         type_safe::optional_ref<const markup::brief_section> brief) const;

    /// \returns The markup containing the index of all modules registered so far.
    /// \requires This function must only be called once.
    /// \notes This function is thread safe.
    std::unique_ptr<markup::module_index> generate() const;

private:
    mutable std::mutex                                         mutex_;
    mutable std::vector<markup::module_documentation::builder> modules_;
};

class comment_registry;

/// Registers all entities in a module for the corresponding module.
void register_module_entities(const module_index& index, const comment_registry& registry,
                              const cppast::cpp_file& file);
} // namespace standardese

#endif // STANDARDESE_INDEX_HPP_INCLUDED
