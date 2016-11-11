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
    file->generate_documentation(p, *doc);
    return {std::move(file), std::move(doc)};
}

md_ptr<md_document> standardese::generate_file_index(index& i, std::string name)
{
    auto doc = md_document::make(std::move(name));

    auto list = md_list::make(*doc, md_list_type::bullet, md_list_delimiter::none, 0, false);
    i.for_each_file([&](const doc_entity& e) {
        auto& paragraph = make_list_item_paragraph(*list);

        auto link = md_link::make(paragraph, "", e.get_unique_name().c_str());
        link->add_entity(md_text::make(*link, e.get_name().c_str()));
        paragraph.add_entity(std::move(link));

    });
    doc->add_entity(std::move(list));

    return doc;
}

namespace
{
    void make_index_item(md_list& list, const doc_entity& e)
    {
        auto& paragraph = make_list_item_paragraph(list);

        // add link to entity
        auto link = md_link::make(paragraph, "", e.get_unique_name().c_str());
        link->add_entity(md_text::make(*link, e.get_index_name().c_str()));
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

    md_ptr<md_list_item> make_group_item(const md_list& list, const char* name)
    {
        auto item = md_list_item::make(list);

        auto paragraph = md_paragraph::make(*item);
        paragraph->add_entity(md_code::make(*item, name));
        item->add_entity(std::move(paragraph));
        item->add_entity(md_list::make_bullet(*paragraph));

        return item;
    }
}

md_ptr<md_document> standardese::generate_entity_index(index& i, std::string name)
{
    auto doc  = md_document::make(std::move(name));
    auto list = md_list::make_bullet(*doc);

    std::map<std::string, md_ptr<md_list_item>> ns_lists;
    i.for_each_namespace_member([&](const doc_entity* ns, const doc_entity& e) {
        if (!ns)
            make_index_item(*list, e);
        else
        {
            auto ns_name = ns->get_index_name();
            auto iter    = ns_lists.find(ns_name.c_str());
            if (iter == ns_lists.end())
            {
                auto item = make_group_item(*list, ns_name.c_str());
                iter      = ns_lists.emplace(ns_name.c_str(), std::move(item)).first;
            }

            auto& item = *iter->second;
            assert(std::next(item.begin())->get_entity_type() == md_entity::list_t);
            make_index_item(static_cast<md_list&>(*std::next(item.begin())), e);
        }
    });

    for (auto& p : ns_lists)
        list->add_entity(std::move(p.second));
    doc->add_entity(std::move(list));

    return doc;
}

md_ptr<md_document> standardese::generate_module_index(index& i, std::string name)
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
            auto item = make_group_item(*list, module_name.c_str());
            iter      = module_lists.emplace(module_name, std::move(item)).first;
        }

        auto& item = *iter->second;
        assert(std::next(item.begin())->get_entity_type() == md_entity::list_t);
        make_index_item(static_cast<md_list&>(*std::next(item.begin())), e);
    });

    for (auto& p : module_lists)
        list->add_entity(std::move(p.second));
    doc->add_entity(std::move(list));

    return doc;
}
