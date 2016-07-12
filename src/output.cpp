// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/output.hpp>

#include <standardese/comment.hpp>
#include <standardese/generator.hpp>
#include <standardese/index.hpp>
#include <standardese/md_inlines.hpp>

using namespace standardese;

namespace
{
    void get_empty_links(std::vector<md_link*>& links, md_entity& cur)
    {
        if (cur.get_entity_type() == md_entity::link_t)
        {
            auto& link = static_cast<md_link&>(cur);
            if (*link.get_destination() == '\0')
                // empty link
                links.push_back(&link);
        }
        else if (is_container(cur.get_entity_type()))
        {
            auto& container = static_cast<md_container&>(cur);
            for (auto& entity : container)
                get_empty_links(links, entity);
        }
    }

    const char* get_entity_name(const md_link& link)
    {
        if (link.begin()->get_entity_type() != md_entity::text_t)
            // must be a text
            return nullptr;
        auto& text = static_cast<const md_text&>(*link.begin());
        return text.get_string();
    }
}

void output::render(const md_document& doc)
{
    auto document = md_ptr<md_document>(static_cast<md_document*>(doc.clone().release()));

    std::vector<md_link*> links;
    get_empty_links(links, *document);

    for (auto link : links)
    {
        auto str = get_entity_name(*link);
        if (!str)
            continue;

        auto& commment = index_->lookup(str);
        link->set_destination((commment.get_output_name() + '.' + format_->extension()).c_str());
    }

    file_output output(prefix_ + document->get_output_name() + '.' + format_->extension());
    format_->render(output, *document);
}
