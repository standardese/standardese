// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_LINKER_HPP_INCLUDED
#define STANDARDESE_LINKER_HPP_INCLUDED

#include <map>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include <type_safe/variant.hpp>

#include <standardese/markup/link.hpp>

namespace cppast
{
class cpp_entity;
class diagnostic_logger;
} // namespace cppast

namespace standardese
{
namespace markup
{
    class document_entity;
} // namespace markup

/// Stores the information about the location of the entity documentation in the output.
///
/// This is used to resolve documentation links.
class linker
{
public:
    void register_external(std::string namespace_name, std::string url);

    /// \effects Registers the given documentation under a certain name.
    /// All unresolved links with that name will resolve to the given documentation.
    /// If `force` is `true`, it will replace a previous registered documentation.
    /// \returns `false` if the link name was used twice.
    /// \notes This function is thread safe.
    bool register_documentation(std::string link_name, const markup::document_entity& document,
                                const markup::block_id& documentation, bool force = false) const;

    /// \returns A reference to the documentation for the given linke name, if there is any.
    /// \notes This function is thread safe.
    type_safe::variant<type_safe::nullvar_t, markup::block_reference, markup::url>
        lookup_documentation(type_safe::optional_ref<const cppast::cpp_entity> context,
                             std::string                                       link_name) const;

private:
    mutable std::mutex                                               mutex_;
    mutable std::unordered_map<std::string, markup::block_reference> map_;

    std::map<std::string, std::string> external_doc_;
};

/// Registers all documentations in a document.
/// \effects Registers every [standardese::markup::documentation_entity]() using its link name.
/// Registers every [cppast::cpp_entity]() that is not documented but would have been documented in
/// that file, using a documented parent's unique name. \notes This function is thread safe.
void register_documentations(const cppast::diagnostic_logger& logger, const linker& l,
                             const markup::document_entity& document);

/// Resolves all unresolved links in a document.
/// \effects For all [standardese::markup::documentation_link]() entities that are not yet resolved,
/// uses the linker to resolve them.
/// \notes This function is *not* thread safe and must be called after the linker is entirely
/// populated.
void resolve_links(const cppast::diagnostic_logger& logger, const linker& l,
                   const markup::document_entity& document);
} // namespace standardese

#endif // STANDARDESE_LINKER_HPP_INCLUDED
