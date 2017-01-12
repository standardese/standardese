// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/generator.hpp>

#include <standardese/doc_entity.hpp>
#include <standardese/index.hpp>
#include <standardese/md_blocks.hpp>
#include <standardese/md_inlines.hpp>
#include <standardese/parser.hpp>
#include <standardese/output.hpp>

using namespace standardese;

standardese::documentation standardese::generate_doc_file(const parser& p, const index& i,
                                                          const cpp_file& f, std::string name)
{
    auto file = doc_file::parse(p, i, std::move(name), f);

    auto doc = md_document::make(std::string("doc_") + file->get_file_name().c_str());
    file->generate_documentation(p, i, *doc);
    return {std::move(file), std::move(doc)};
}

namespace
{
    using standardese::index;

    void make_index_item(const index& idx, md_list& list, const doc_entity& e, bool full_name)
    {
        auto& paragraph = make_list_item_paragraph(list);

        // add link to entity
        auto link = md_link::make(paragraph, "", e.get_unique_name().c_str());
        link->add_entity(md_text::make(*link, e.get_index_name(full_name, true).c_str()));
        paragraph.add_entity(std::move(link));

        // add brief comment to it
        auto comment = e.has_comment() ? &e.get_comment() : nullptr;
        if (e.has_parent() && e.get_parent().get_entity_type() == doc_entity::member_group_t)
            comment = &e.get_parent().get_comment();
        if (comment && !comment->get_content().get_brief().empty())
        {
            paragraph.add_entity(md_text::make(paragraph, " - "));
            for (auto& child : comment->get_content().get_brief())
                paragraph.add_entity(child.clone(paragraph));
            normalize_urls(idx, paragraph, &e);
        }
    }

    md_ptr<md_list_item> make_group_item(const index& idx, const char* file_name,
                                         const md_list& list, const char* name, unsigned level,
                                         bool code, const doc_entity* entity,
                                         const md_comment* comment)
    {
        auto item = md_list_item::make(list);

        md_ptr<md_container> heading;
        if (level == 0u)
            heading = md_paragraph::make(*item);
        else
            heading = md_heading::make(*item, level);

        // add name
        if (code)
            heading->add_entity(md_code::make(*item, name));
        else
            heading->add_entity(md_text::make(*item, name));

        // add anchor
        if (entity)
        {
            heading->add_entity(idx.get_linker().get_anchor(*entity, *heading));
            idx.get_linker().register_anchor(entity->get_unique_name().c_str(), file_name);
        }
        else
        {
            heading->add_entity(md_anchor::make(*heading, name));
            idx.get_linker().register_anchor(name, file_name);
        }

        // add brief
        auto brief = comment ? &comment->get_brief() : nullptr;
        if (brief && !brief->empty())
        {
            heading->add_entity(md_text::make(*heading, " - "));
            for (auto& child : *brief)
                heading->add_entity(child.clone(*heading));
            normalize_urls(idx, *heading, entity);
        }

        item->add_entity(std::move(heading));

        // add rest of comment
        if (comment)
        {
            for (auto& child : *comment)
                if (child.get_entity_type() == md_entity::paragraph_t
                    && static_cast<const md_paragraph&>(child).get_section_type()
                           == section_type::brief)
                    continue;
                else
                    item->add_entity(child.clone(*item));
        }

        // add list
        item->add_entity(md_list::make_bullet(*item));

        return item;
    }
}

documentation standardese::generate_file_index(index& i, std::string name)
{
    auto doc = md_document::make(std::move(name));

    auto list = md_list::make(*doc, md_list_type::bullet, md_list_delimiter::none, 0, false);
    auto size = 0u;
    i.for_each_file([&](const doc_entity& e) {
        make_index_item(i, *list, e, false);
        ++size;
    });

    assert(size != 0u);
    if (size == 1u)
        return documentation(nullptr, nullptr);

    doc->add_entity(std::move(list));
    auto entity = detail::make_doc_ptr<doc_index>(*doc, doc->get_output_name());
    return documentation(std::move(entity), std::move(doc));
}

documentation standardese::generate_entity_index(index& i, std::string name)
{
    auto doc  = md_document::make(std::move(name));
    auto list = md_list::make_bullet(*doc);

    std::map<std::string, md_ptr<md_list_item>> ns_lists;
    i.for_each_namespace_member([&](const doc_entity* ns, const doc_entity& e) {
        if (!ns)
            make_index_item(i, *list, e, false);
        else
        {
            auto ns_name = ns->get_index_name(false, true);
            auto iter    = ns_lists.find(ns_name.c_str());
            if (iter == ns_lists.end())
            {
                auto comment = ns->has_comment() ? &ns->get_comment().get_content() : nullptr;
                auto item    = make_group_item(i, doc->get_output_name().c_str(), *list,
                                            ns_name.c_str(), 0u, true, ns, comment);
                iter = ns_lists.emplace(ns_name.c_str(), std::move(item)).first;
            }

            auto& item = *iter->second;
            assert(item.back().get_entity_type() == md_entity::list_t);
            make_index_item(i, static_cast<md_list&>(item.back()), e, false);
        }
    });

    for (auto& p : ns_lists)
        list->add_entity(std::move(p.second));
    doc->add_entity(std::move(list));

    auto entity = detail::make_doc_ptr<doc_index>(*doc, doc->get_output_name());
    return documentation(std::move(entity), std::move(doc));
}

documentation standardese::generate_module_index(const parser& p, index& i, std::string name)
{
    auto doc  = md_document::make(std::move(name));
    auto list = md_list::make_bullet(*doc);

    std::map<std::string, md_ptr<md_list_item>> module_lists;
    i.for_each_namespace_member([&](const doc_entity*, const doc_entity& e) {
        if (!e.in_module())
            return;

        auto& module_name = e.get_module();
        auto  iter        = module_lists.find(module_name);
        if (iter == module_lists.end())
        {
            auto comment = p.get_comment_registry().lookup_comment(module_name);
            auto item =
                make_group_item(i, doc->get_output_name().c_str(), *list, module_name.c_str(), 2u,
                                false, nullptr, comment ? &comment->get_content() : nullptr);
            iter = module_lists.emplace(module_name, std::move(item)).first;
        }

        auto& item = *iter->second;
        assert(item.back().get_entity_type() == md_entity::list_t);
        make_index_item(i, static_cast<md_list&>(item.back()), e, true);
    });

    if (module_lists.empty())
        return documentation(nullptr, nullptr);

    for (auto& p : module_lists)
        list->add_entity(std::move(p.second));
    doc->add_entity(std::move(list));

    auto entity = detail::make_doc_ptr<doc_index>(*doc, doc->get_output_name());
    return documentation(std::move(entity), std::move(doc));
}
