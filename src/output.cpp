// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/output.hpp>

#include <spdlog/logger.h>

#include <standardese/comment.hpp>
#include <standardese/generator.hpp>
#include <standardese/index.hpp>
#include <standardese/linker.hpp>
#include <standardese/md_inlines.hpp>

using namespace standardese;

namespace
{
    const char link_prefix[] = "standardese://";

    void get_standardese_links(std::vector<md_link*>& links, md_entity& cur)
    {
        if (cur.get_entity_type() == md_entity::link_t)
        {
            auto& link = static_cast<md_link&>(cur);
            if (*link.get_destination() == '\0')
                // empty link
                links.push_back(&link);
            else if (std::strncmp(link.get_destination(), link_prefix, sizeof(link_prefix) - 1)
                     == 0)
                // standardese link
                links.push_back(&link);
        }
        else if (is_container(cur.get_entity_type()))
        {
            auto& container = static_cast<md_container&>(cur);
            for (auto& entity : container)
                get_standardese_links(links, entity);
        }
    }

    const char* get_entity_name(const md_link& link)
    {
        if (*link.get_destination())
        {
            assert(std::strncmp(link.get_destination(), link_prefix, sizeof(link_prefix) - 1) == 0);
            return link.get_destination() + sizeof(link_prefix) - 1;
        }
        else if (*link.get_title())
            return link.get_title();
        else if (link.begin()->get_entity_type() != md_entity::text_t)
            // must be a text
            return nullptr;
        auto& text = static_cast<const md_text&>(*link.begin());
        return text.get_string();
    }
}

void standardese::resolve_urls(const std::shared_ptr<spdlog::logger>& logger, const linker& l,
                               const index& i, md_document& document, const char* extension)
{
    std::vector<md_link*> links;
    get_standardese_links(links, document);

    for (auto link : links)
    {
        auto str = get_entity_name(*link);
        if (!str)
            continue;

        auto destination = l.get_url(i, str, extension);
        if (destination.empty())
        {
            logger->warn("unable to resolve link to an entity named '{}'", str);
            continue;
        }
        link->set_destination(destination.c_str());
    }
}

void output::render(const std::shared_ptr<spdlog::logger>& logger, const md_document& doc,
                    const char* output_extension)
{
    auto document = md_ptr<md_document>(static_cast<md_document*>(doc.clone().release()));
    resolve_urls(logger, *linker_, *index_, *document,
                 output_extension ? output_extension : format_->extension());

    file_output output(prefix_ + document->get_output_name() + '.' + format_->extension());
    format_->render(output, *document);
}
