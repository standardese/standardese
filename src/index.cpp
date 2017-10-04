// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/index.hpp>

#include <algorithm>
#include <cassert>
#include <cppast/cpp_file.hpp>
#include <cppast/cpp_preprocessor.hpp>
#include <cppast/cpp_namespace.hpp>
#include <type_safe/downcast.hpp>

#include <standardese/markup/document.hpp>
#include <standardese/markup/link.hpp>
#include <standardese/markup/code_block.hpp>
#include <standardese/markup/entity_kind.hpp>
#include <standardese/comment.hpp>
#include <standardese/doc_entity.hpp>

#include "entity_visitor.hpp"

using namespace standardese;

void entity_index::insert(entity e) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto                        range =
        std::equal_range(entities_.begin(), entities_.end(), e,
                         [](const entity_index::entity& lhs, const entity_index::entity& rhs) {
                             return lhs.scope + lhs.name < rhs.scope + rhs.name;
                         });
    if (range.first == range.second)
        entities_.insert(range.first, std::move(e));
    else
    {
        assert(std::next(range.first) == range.second);
        auto& inserted = *range.first;
        if (auto builder = inserted.doc.optional_value(
                type_safe::variant_type<markup::namespace_documentation::builder>{}))
        {
            auto& e_builder =
                e.doc.value(type_safe::variant_type<markup::namespace_documentation::builder>{});
            if (!builder.value().has_documentation() && e_builder.has_documentation())
                inserted.doc = std::move(e.doc);
        }
    }
}

namespace
{
    std::unique_ptr<markup::entity_index_item> get_entity_entry(
        const std::string& name, std::string link_name,
        type_safe::optional_ref<const markup::brief_section> brief)
    {
        auto link = markup::documentation_link::builder(link_name)
                        .add_child(markup::code::build(name))
                        .finish();
        auto term = markup::term::build(std::move(link));

        if (brief)
        {
            markup::description::builder description;
            for (auto& child : brief.value())
                description.add_child(markup::clone(child));

            return markup::entity_index_item::build(markup::block_id(std::move(link_name)),
                                                    std::move(term), description.finish());
        }
        else
            return markup::entity_index_item::build(markup::block_id(std::move(link_name)),
                                                    std::move(term));
    }

    std::string get_scope(const cppast::cpp_entity& e)
    {
        std::string result;
        for (auto parent = e.parent(); parent; parent = parent.value().parent())
            if (parent.value().kind() == cppast::cpp_namespace::kind())
                result = parent.value().name() + "::" + result;
        return result;
    }
}

void entity_index::register_entity(std::string link_name, const cppast::cpp_entity& e,
                                   type_safe::optional_ref<const markup::brief_section> brief) const
{
    assert(e.kind() != cppast::cpp_file::kind() && e.kind() != cppast::cpp_namespace::kind());
    if (e.kind() != cppast::cpp_include_directive::kind()) // don't insert includes
        insert(entity(get_entity_entry(e.name(), std::move(link_name), brief), e.name(),
                      get_scope(e)));
}

void entity_index::register_namespace(const cppast::cpp_namespace&             ns,
                                      markup::namespace_documentation::builder doc) const
{
    insert(entity(std::move(doc), ns.name(), get_scope(ns)));
}

namespace
{
    struct nested_list_builder
    {
        std::string scope;
        type_safe::variant<type_safe::object_ref<markup::entity_index::builder>,
                           markup::namespace_documentation::builder>
            builder;

        void pop(nested_list_builder& previous)
        {
            struct lambda
            {
                void operator()(type_safe::object_ref<markup::entity_index::builder> builder,
                                std::unique_ptr<markup::namespace_documentation>     doc)
                {
                    builder->add_child(std::move(doc));
                }

                void operator()(markup::namespace_documentation::builder&        builder,
                                std::unique_ptr<markup::namespace_documentation> doc)
                {
                    builder.add_child(std::move(doc));
                }
            };
            type_safe::with(previous.builder, lambda{},
                            builder
                                .value(type_safe::variant_type<
                                       markup::namespace_documentation::builder>{})
                                .finish());
        }

        void add_item(std::unique_ptr<markup::entity_index_item> item)
        {
            struct lambda
            {
                void operator()(type_safe::object_ref<markup::entity_index::builder> builder,
                                std::unique_ptr<markup::entity_index_item>           item)
                {
                    builder->add_child(std::move(item));
                }

                void operator()(markup::namespace_documentation::builder&  builder,
                                std::unique_ptr<markup::entity_index_item> item)
                {
                    builder.add_child(std::move(item));
                }
            };
            type_safe::with(builder, lambda{}, std::move(item));
        }
    };
}

std::unique_ptr<markup::entity_index> entity_index::generate() const
{
    markup::entity_index::builder builder(
        markup::heading::build(markup::block_id(), "Project index"));

    std::vector<nested_list_builder> lists;
    lists.push_back(nested_list_builder{"", type_safe::ref(builder)});

    std::unique_lock<std::mutex> lock(mutex_);
    for (auto& entity : entities_)
    {
        // find matching parent
        while (entity.scope != (lists.back().scope.empty() ? "" : lists.back().scope + "::"))
        {
            auto ns = std::move(lists.back());
            lists.pop_back();
            ns.pop(lists.back());
        }

        if (auto ns = entity.doc.optional_value(
                type_safe::variant_type<markup::namespace_documentation::builder>{}))
            // we've got a namespace
            lists.push_back(nested_list_builder{entity.scope + entity.name, std::move(ns.value())});
        else
            // normal entity
            lists.back().add_item(std::move(entity.doc.value(
                type_safe::variant_type<std::unique_ptr<markup::entity_index_item>>{})));
    }
    lock.unlock();

    while (!lists.empty())
    {
        auto ns = std::move(lists.back());
        lists.pop_back();
        if (!lists.empty())
            ns.pop(lists.back());
    }

    return builder.finish();
}

void standardese::register_index_entities(const entity_index& index, const cppast::cpp_file& file)
{
    detail::visit_namespace_level(file,
                                  [&](const cppast::cpp_entity& entity) {
                                      auto doc_e =
                                          static_cast<const doc_entity*>(entity.user_data());
                                      if (doc_e && !doc_e->is_excluded())
                                      {
                                          auto brief_section = doc_e->comment().map(
                                              [](const comment::doc_comment& comment) {
                                                  return comment.brief_section();
                                              });
                                          index.register_entity(doc_e->link_name(), entity,
                                                                brief_section);
                                      }
                                  },
                                  [&](const cppast::cpp_namespace& ns) {
                                      auto doc_e = static_cast<const doc_entity*>(ns.user_data());
                                      if (!doc_e->is_excluded())
                                          // it's not an excluded entity, so register it
                                          index.register_namespace(ns,
                                                                   static_cast<
                                                                       const doc_cpp_namespace*>(
                                                                       doc_e)
                                                                       ->get_builder());
                                  });
}

void file_index::register_file(std::string link_name, std::string file_name,
                               type_safe::optional_ref<const markup::brief_section> brief) const
{
    file_index::file f(file_name, get_entity_entry(file_name, link_name, brief));

    std::lock_guard<std::mutex> lock(mutex_);
    auto                        range = std::equal_range(files_.begin(), files_.end(), f,
                                  [](const file_index::file& lhs, const file_index::file& rhs) {
                                      return lhs.name < rhs.name;
                                  });
    if (range.first == range.second)
        files_.insert(range.first, std::move(f));
}

std::unique_ptr<markup::file_index> file_index::generate() const
{
    markup::file_index::builder builder(
        markup::heading::build(markup::block_id(), "Project files"));

    std::unique_lock<std::mutex> lock(mutex_);
    for (auto& file : files_)
        builder.add_child(std::move(file.doc));
    lock.unlock();

    return builder.finish();
}

void module_index::register_module(markup::module_documentation::builder doc) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto                        range = std::equal_range(modules_.begin(), modules_.end(), doc,
                                  [](const markup::module_documentation::builder& lhs,
                                     const markup::module_documentation::builder& rhs) {
                                      return lhs.id().as_str() < rhs.id().as_str();
                                  });
    if (range.first == range.second)
        modules_.insert(range.first, std::move(doc));
}

bool module_index::register_entity(std::string module, std::string link_name,
                                   const cppast::cpp_entity&                            entity,
                                   type_safe::optional_ref<const markup::brief_section> brief) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto                        iter = std::lower_bound(modules_.begin(), modules_.end(), module,
                                 [](const markup::module_documentation::builder& lhs,
                                    const std::string& rhs) { return lhs.id().as_str() < rhs; });
    if (iter == modules_.end() || iter->id().as_str() != module)
        return false;
    iter->add_child(get_entity_entry(entity.name(), std::move(link_name), std::move(brief)));
    return true;
}

std::unique_ptr<markup::module_index> module_index::generate() const
{
    markup::module_index::builder builder(
        markup::heading::build(markup::block_id(), "Project modules"));

    std::unique_lock<std::mutex> lock(mutex_);
    for (auto& module : modules_)
        builder.add_child(module.finish());
    lock.unlock();

    return builder.finish();
}

void standardese::register_module_entities(const module_index&     index,
                                           const comment_registry& registry,
                                           const cppast::cpp_file& file)
{
    auto get_module = [&](const cppast::cpp_entity& e) -> type_safe::optional<std::string> {
        if (auto doc_e = static_cast<const doc_entity*>(e.user_data()))
        {
            if (doc_e->comment())
                return doc_e->comment().value().metadata().module();
        }

        return type_safe::nullopt;
    };

    auto register_entity = [&](std::string module, const cppast::cpp_entity& e) {
        assert(e.user_data());
        auto& doc_e = *static_cast<const doc_entity*>(e.user_data());
        return index.register_entity(std::move(module), doc_e.link_name(), e,
                                     doc_e.comment().value().brief_section());
    };

    auto get_module_doc = [&](const std::string& name) {
        markup::module_documentation::builder builder(markup::block_id(name),
                                                      markup::heading::builder(markup::block_id())
                                                          .add_child(markup::text::build("Module "))
                                                          .add_child(markup::code::build(name))
                                                          .finish());

        if (auto module_comment = registry.get_comment(name))
            comment::set_sections(builder, module_comment.value());

        return builder;
    };

    cppast::visit(file, [&](const cppast::cpp_entity& e, const cppast::visitor_info& info) {
        if (info.event != cppast::visitor_info::container_entity_exit)
        {
            auto module = get_module(e);
            if (module && !register_entity(module.value(), e))
            {
                // need to register module
                auto module_doc = get_module_doc(module.value());
                index.register_module(std::move(module_doc));

                // can register again now
                auto result = register_entity(module.value(), e);
                assert(result);
            }
        }

        return true;
    });
}
