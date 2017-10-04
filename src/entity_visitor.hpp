// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_ENTITY_VISITOR_HPP_INCLUDED
#define STANDARDESE_ENTITY_VISITOR_HPP_INCLUDED

#include <cppast/cpp_file.hpp>
#include <cppast/cpp_namespace.hpp>
#include <cppast/visitor.hpp>

namespace standardese
{
    namespace detail
    {
        template <typename EntityFunc, typename NamespaceFunc>
        void visit_namespace_level(const cppast::cpp_file& file, EntityFunc ef, NamespaceFunc nf)
        {
            for (auto& child : file)
                // visit all children to filter out namespaces before processing
                cppast::visit(child, [&](const cppast::cpp_entity&   e,
                                         const cppast::visitor_info& info) {
                    switch (info.event)
                    {
                    case cppast::visitor_info::leaf_entity:
                        ef(e);
                        return true; // continue with other entities

                    case cppast::visitor_info::container_entity_enter:
                        if (e.kind() == cppast::cpp_entity_kind::namespace_t)
                        {
                            nf(static_cast<const cppast::cpp_namespace&>(e));
                            return true; // continue with children
                        }
                        else if (e.kind() == cppast::cpp_entity_kind::language_linkage_t)
                            return true; // continue with children
                        else
                        {
                            ef(e);
                            return false; // don't visit children
                        }
                    case cppast::visitor_info::container_entity_exit:
                        // did this one already
                        return true; // continue with other entities
                    }

                    assert(false);
                    return false;
                });
        }

        template <typename EntityFunc>
        void visit_namespace_level(const cppast::cpp_file& file, EntityFunc ef)
        {
            visit_namespace_level(file, ef, [](const cppast::cpp_namespace&) {});
        }

        template <typename Func>
        void visit_children(const cppast::cpp_entity& entity, Func f)
        {
            cppast::visit(entity, [&](const cppast::cpp_entity&   child,
                                      const cppast::visitor_info& info) {
                if (&entity == &child)
                    // parent entity
                    // if container: true means visit children (what we want)
                    // else: true means continue visit (which will do nothing)
                    return true;
                else if (cppast::is_templated(child) || cppast::is_friended(child)
                         || child.kind() == cppast::cpp_entity_kind::language_linkage_t)
                    // continue with children of those entities
                    return true;
                else if (info.event == cppast::visitor_info::container_entity_exit)
                    // already done, continue
                    return true;
                else if (info.event == cppast::visitor_info::container_entity_enter)
                {
                    f(child, info.access);
                    return false; // don't visit children
                }
                else if (info.event == cppast::visitor_info::leaf_entity)
                {
                    f(child, info.access);
                    return true; //continue
                }
                else
                    assert(false);
                return false;
            });
        }
    } // namespace detail
} // namespace standardese

#endif // STANDARDESE_ENTITY_VISITOR_HPP_INCLUDED
