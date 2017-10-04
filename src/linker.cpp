// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/linker.hpp>

#include <algorithm>
#include <cassert>

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_file.hpp>
#include <cppast/cpp_namespace.hpp>
#include <cppast/visitor.hpp>

#include <standardese/markup/document.hpp>
#include <standardese/markup/documentation.hpp>
#include <standardese/markup/entity_kind.hpp>
#include <standardese/markup/link.hpp>
#include <standardese/doc_entity.hpp>
#include <standardese/logger.hpp>

#include "get_special_entity.hpp"

using namespace standardese;

namespace
{
    bool is_relative(const std::string& link_name)
    {
        return !link_name.empty() && (link_name.front() == '*' || link_name.front() == '?');
    }

    std::string process_link_name(std::string name)
    {
        name.erase(std::remove(name.begin(), name.end(), ' '), name.end());

        if (name.size() >= 2u && name.rbegin()[1] == '(' && name.rbegin()[0] == ')')
        {
            // ends with ()
            name.pop_back();
            name.pop_back();
        }

        if (is_relative(name))
            return name.substr(1);
        else
            return name;
    }

    std::string short_link_name(const std::string& name)
    {
        std::string result;

        auto skip = false;
        for (auto ptr = name.c_str(); *ptr; ++ptr)
        {
            auto c = *ptr;
            if (c == '(')
            {
                result += '(';
                skip = true;
            }
            else if (c == '.')
            {
                if (ptr[1] == '.')
                {
                    // ... token
                    if (ptr[2] == '.')
                        ptr += 2; // ... token
                    else
                        ptr += 1; // .. from file
                }
                else
                {
                    // next token is '.' separator for parameter
                    result += ").";
                    skip = false;
                }
            }
            else if (c == '<')
                skip = true;
            else if (c == '>')
                skip = false;
            else if (!skip)
                result += c;
        }

        if (result.back() == '(')
            result.pop_back();

        return result;
    }
}

bool linker::register_documentation(std::string link_name, const markup::document_entity& document,
                                    const markup::block_id& documentation, bool force) const
{
    auto ref = markup::block_reference(document.output_name(), documentation);

    link_name       = process_link_name(std::move(link_name));
    auto short_name = short_link_name(link_name);

    std::lock_guard<std::mutex> lock(mutex_);

    // insert long name
    auto result = map_.emplace(std::move(link_name), ref);
    if (!result.second) // not inserted
    {
        if (force)
            result.first->second = ref; // override anyway
        else
            return false;
    }

    // insert short name
    if (short_name != result.first->first)
    {
        result = map_.emplace(std::move(short_name), ref);
        if (!result.second)
        {
            if (force)
                result.first->second = std::move(ref);
            else
                // duplicate, erase first one as well
                map_.erase(result.first);
        }
    }

    return true;
}

namespace
{
    std::string get_scope_name(const cppast::cpp_entity& entity)
    {
        auto scope      = entity.scope_name();
        auto scope_name = scope.map(&cppast::cpp_scope_name::name);
        return type_safe::copy(scope_name).value_or("");
    }

    std::string get_entity_scope(const cppast::cpp_entity& entity)
    {
        std::string result;
        for (auto cur = entity.parent(); cur; cur = cur.value().parent())
        {
            auto cur_scope = get_scope_name(cur.value());
            if (!cur_scope.empty())
                result = cur_scope + "::" + result;
        }
        return result;
    }
}

type_safe::optional_ref<const markup::block_reference> linker::lookup_documentation(
    type_safe::optional_ref<const cppast::cpp_entity> context, std::string link_name) const
{
    auto relative = is_relative(link_name);
    link_name     = process_link_name(std::move(link_name));

    auto do_lookup = [&](const std::string& link_name)
        -> type_safe::optional_ref<const markup::block_reference> {
        std::lock_guard<std::mutex> lock(mutex_);
        auto                        iter = map_.find(process_link_name(link_name));
        if (iter == map_.end())
            return nullptr;
        return type_safe::ref(iter->second);
    };

    if (!relative)
        return do_lookup(link_name);
    else
    {
        while (context)
        {
            if (auto result = do_lookup(get_entity_scope(context.value()) + link_name))
                return result;

            // go to parent
            context = context.value().parent();
        }

        return nullptr;
    }
}

namespace
{
    template <class FileVisitor>
    void visit_documentations(const markup::document_entity& document,
                              const FileVisitor&             file_visitor)
    {
        markup::visit(document, [&](const markup::entity& e) {
            if (e.kind() == markup::entity_kind::file_documentation)
                file_visitor(static_cast<const markup::file_documentation&>(e));
            // note: no need to handle entity_documentation
        });
    }

    type_safe::optional_ref<const doc_entity> get_doc_entity(const cppast::cpp_entity& e)
    {
        auto user_data = e.user_data();
        if (!user_data)
            return type_safe::nullopt;

        auto& doc_e = *static_cast<const doc_entity*>(user_data);
        if (doc_e.is_excluded())
            return type_safe::nullopt;
        else
            return type_safe::ref(doc_e);
    }
}

void standardese::register_documentations(const cppast::diagnostic_logger& logger, const linker& l,
                                          const markup::document_entity& document)
{
    auto register_doc = [&](const cppast::cpp_entity& e) {
        if (auto doc_e = get_doc_entity(e))
        {
            auto result = l.register_documentation(doc_e.value().link_name(), document,
                                                   doc_e.value().get_documentation_id(),
                                                   cppast::is_definition(e));
            if (!result)
                logger.log("standardese linker",
                           make_diagnostic(cppast::source_location::make_entity(
                                               doc_e.value().get_documentation_id().as_str()),
                                           "duplicate registration of link name '",
                                           doc_e.value().link_name(), "'"));
        }
    };

    visit_documentations(document, [&](const markup::file_documentation& file) {
        cppast::visit(file.file(),
                      [&](const cppast::cpp_entity& e, const cppast::visitor_info& info) {
                          if (info.event != cppast::visitor_info::container_entity_exit
                              && !cppast::is_templated(e) && !cppast::is_friended(e)
                              && e.kind() != cppast::cpp_namespace::kind()) // if not already done
                          {
                              register_doc(e);

                              // handle inline entities
                              if (auto func = detail::get_function(e))
                                  for (auto& param : func.value().parameters())
                                      register_doc(param);
                              if (auto templ = detail::get_template(e))
                                  for (auto& param : templ.value().parameters())
                                      register_doc(param);
                              if (auto c = detail::get_class(e))
                                  for (auto& base : c.value().bases())
                                      register_doc(base);
                          }

                          return true;
                      });
    });
}

namespace
{
    cppast::source_location get_location(const markup::document_entity& document,
                                         const markup::internal_link&   link)
    {
        auto result = cppast::source_location::make_file(document.output_name().name());

        for (auto parent = link.parent(); parent; parent = parent.value().parent())
            if (parent.value().kind() == markup::entity_kind::entity_documentation)
            {
                result.entity =
                    static_cast<const markup::entity_documentation&>(parent.value()).id().as_str();
                break;
            }

        return result;
    }
}

void standardese::resolve_links(const cppast::diagnostic_logger& logger, const linker& l,
                                const markup::document_entity& document)
{
    auto get_context =
        [](const markup::entity& entity) -> type_safe::optional_ref<const cppast::cpp_entity> {
        if (entity.kind() == markup::entity_kind::file_documentation)
            return type_safe::opt_ref(
                &static_cast<const markup::file_documentation&>(entity).file());
        else if (entity.kind() == markup::entity_kind::entity_documentation)
            return type_safe::opt_ref(
                &static_cast<const markup::entity_documentation&>(entity).entity());
        else
            return nullptr;
    };

    auto get_documentation_block = [](const markup::entity& entity) {
        for (auto cur = entity.parent(); cur; cur = cur.value().parent())
            if (markup::is_documentation(cur.value().kind()))
                return static_cast<const markup::documentation_entity&>(cur.value()).id();
        assert(false);
    };

    type_safe::optional_ref<const cppast::cpp_entity> context;
    markup::visit(document, [&](const markup::entity& entity) {
        if (entity.kind() == markup::entity_kind::internal_link)
        {
            auto& link = static_cast<const markup::internal_link&>(entity);
            if (auto unresolved = link.unresolved_destination())
            {
                auto destination = l.lookup_documentation(context, unresolved.value());
                if (destination)
                {
                    auto same_document = !destination.value().document()
                                         || destination.value().document().value().name()
                                                == document.output_name().name();
                    if (!same_document
                        || destination.value().id().as_str()
                               != get_documentation_block(entity).as_str())
                        // only resolve if points to something different
                        link.resolve_destination(destination.value());
                }
                else
                    logger.log("standardese linker",
                               make_diagnostic(get_location(document, link),
                                               "unresolved link name '", unresolved.value(), '\''));
            }
        }
        else if (auto new_context = get_context(entity))
            context = new_context;
    });
}
