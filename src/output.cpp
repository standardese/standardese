// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/output.hpp>

#include <spdlog/logger.h>

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
        if (*link.get_title())
            return link.get_title();
        else if (link.begin()->get_entity_type() != md_entity::text_t)
            // must be a text
            return nullptr;
        auto& text = static_cast<const md_text&>(*link.begin());
        return text.get_string();
    }

    std::string get_destination(const md_comment& comment, const char* extension,
                                const char* output_extension)
    {
        return comment.get_output_name() + '.' + (output_extension ? output_extension : extension)
               + '#' + comment.get_unique_name();
    }
}

void output::render(const std::shared_ptr<spdlog::logger>& logger, const md_document& doc,
                    const char* output_extension)
{
    auto document = md_ptr<md_document>(static_cast<md_document*>(doc.clone().release()));

    std::vector<md_link*> links;
    get_empty_links(links, *document);

    for (auto link : links)
    {
        auto str = get_entity_name(*link);
        if (!str)
            continue;

        auto comment = index_->try_lookup(str);
        if (!comment)
        {
            logger->warn("unable to resolve link to an entity named '{}'", str);
            continue;
        }
        link->set_destination(
            get_destination(*comment, format_->extension(), output_extension).c_str());
    }

    file_output output(prefix_ + document->get_output_name() + '.' + format_->extension());
    format_->render(output, *document);
}
