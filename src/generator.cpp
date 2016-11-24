// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/generator.hpp>

#include <standardese/doc_entity.hpp>
#include <standardese/index.hpp>
#include <standardese/md_blocks.hpp>
#include <standardese/md_inlines.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

standardese::documentation standardese::generate_doc_file(const parser& p, const index& i,
                                                          const cpp_file& f, std::string name)
{
    auto file = doc_file::parse(p, i, std::move(name), f);

    auto doc = md_document::make(file->get_file_name().c_str());
    file->generate_documentation(p, i, *doc);
    return {std::move(file), std::move(doc)};
}

namespace
{
    void make_index_item(md_list& list, const doc_entity& e, bool full_name)
    {
        auto& paragraph = make_list_item_paragraph(list);

        // add link to entity
        auto link = md_link::make(paragraph, "", e.get_unique_name().c_str());
        link->add_entity(md_text::make(*link, e.get_index_name(full_name).c_str()));
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
        }
    }

    md_ptr<md_list_item> make_group_item(const md_list& list, const char* name, unsigned level,
                                         bool code, const md_paragraph* brief)
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

        // add brief
        if (brief && !brief->empty())
        {
            heading->add_entity(md_text::make(*heading, " - "));
            for (auto& child : *brief)
                heading->add_entity(child.clone(*heading));
        }

        item->add_entity(std::move(heading));

        // add list
        item->add_entity(md_list::make_bullet(*heading));

        return item;
    }
}

md_ptr<md_document> standardese::generate_file_index(index& i, std::string name)
{
    auto doc = md_document::make(std::move(name));

    auto list = md_list::make(*doc, md_list_type::bullet, md_list_delimiter::none, 0, false);
    auto size = 0u;
    i.for_each_file([&](const doc_entity& e) {
        make_index_item(*list, e, false);
        ++size;
    });

    assert(size != 0u);
    if (size == 1u)
        return nullptr;

    doc->add_entity(std::move(list));
    return doc;
}

md_ptr<md_document> standardese::generate_entity_index(index& i, std::string name)
{
    auto doc  = md_document::make(std::move(name));
    auto list = md_list::make_bullet(*doc);

    std::map<std::string, md_ptr<md_list_item>> ns_lists;
    i.for_each_namespace_member([&](const doc_entity* ns, const doc_entity& e) {
        if (!ns)
            make_index_item(*list, e, false);
        else
        {
            auto ns_name = ns->get_index_name(false);
            auto iter    = ns_lists.find(ns_name.c_str());
            if (iter == ns_lists.end())
            {
                auto brief =
                    ns->has_comment() ? &ns->get_comment().get_content().get_brief() : nullptr;
                auto item = make_group_item(*list, ns_name.c_str(), 0u, true, brief);
                iter      = ns_lists.emplace(ns_name.c_str(), std::move(item)).first;
            }

            auto& item = *iter->second;
            assert(std::next(item.begin())->get_entity_type() == md_entity::list_t);
            make_index_item(static_cast<md_list&>(*std::next(item.begin())), e, false);
        }
    });

    for (auto& p : ns_lists)
        list->add_entity(std::move(p.second));
    doc->add_entity(std::move(list));

    return doc;
}

md_ptr<md_document> standardese::generate_module_index(const parser& p, index& i, std::string name)
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
            auto brief   = comment ? &comment->get_content().get_brief() : nullptr;
            auto item    = make_group_item(*list, module_name.c_str(), 2u, false, brief);
            iter         = module_lists.emplace(module_name, std::move(item)).first;
        }

        auto& item = *iter->second;
        assert(std::next(item.begin())->get_entity_type() == md_entity::list_t);
        make_index_item(static_cast<md_list&>(*std::next(item.begin())), e, true);
    });

    if (module_lists.empty())
        return nullptr;

    for (auto& p : module_lists)
        list->add_entity(std::move(p.second));
    doc->add_entity(std::move(list));

    return doc;
}
