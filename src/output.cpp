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
    using standardese::index;

    const char link_prefix[] = "standardese://";

    void get_standardese_links(std::vector<md_link*>& links, md_entity& cur,
                               bool only_empty = false)
    {
        if (cur.get_entity_type() == md_entity::link_t)
        {
            auto& link = static_cast<md_link&>(cur);
            if (*link.get_destination() == '\0')
                // empty link
                links.push_back(&link);
            else if (!only_empty
                     && std::strncmp(link.get_destination(), link_prefix, sizeof(link_prefix) - 1)
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

    std::string get_entity_name(const md_link& link)
    {
        if (*link.get_destination())
        {
            assert(std::strncmp(link.get_destination(), link_prefix, sizeof(link_prefix) - 1) == 0);
            std::string result = link.get_destination() + sizeof(link_prefix) - 1;
            result.pop_back();
            return result;
        }
        else if (*link.get_title())
            return link.get_title();
        else if (link.begin()->get_entity_type() != md_entity::text_t)
            // must be a text
            return "";
        auto& text = static_cast<const md_text&>(*link.begin());
        return text.get_string();
    }

    void resolve_urls(const std::shared_ptr<spdlog::logger>& logger, const linker& l,
                      const index& i, md_document& document, const char* extension)
    {
        std::vector<md_link*> links;
        get_standardese_links(links, document);

        for (auto link : links)
        {
            auto str = get_entity_name(*link);
            if (str.empty())
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
}

void standardese::normalize_urls(md_document& document)
{
    std::vector<md_link*> links;
    get_standardese_links(links, document, true);

    for (auto link : links)
    {
        auto str = get_entity_name(*link);
        if (str.empty())
            continue;

        link->set_destination(("standardese://" + str + '/').c_str());
    }
}

raw_document::raw_document(path fname, std::string text)
: file_name(std::move(fname)), text(std::move(text))
{
    auto idx = file_name.rfind('.');
    if (idx != path::npos)
    {
        file_extension = file_name.substr(idx + 1);
        file_name.erase(idx);
    }
}

void output::render(const std::shared_ptr<spdlog::logger>& logger, const md_document& doc,
                    const char* output_extension)
{
    if (!output_extension)
        output_extension = format_->extension();

    auto document = md_ptr<md_document>(static_cast<md_document*>(doc.clone().release()));
    resolve_urls(logger, index_->get_linker(), *index_, *document, output_extension);

    file_output output(prefix_ + document->get_output_name() + '.' + output_extension);
    format_->render(output, *document);
}

void output::render_template(const std::shared_ptr<spdlog::logger>& logger,
                             const template_file& templ, const doc_file& file,
                             const std::string& output_name, const char* output_extension)
{
    if (!output_extension)
        output_extension = format_->extension();

    auto document           = process_template(*parser_, *index_, templ, format_, &file);
    document.file_name      = output_name;
    document.file_extension = output_extension;

    render_raw(logger, document);
}

void output::render_raw(const std::shared_ptr<spdlog::logger>& logger, const raw_document& document)
{
    auto extension =
        document.file_extension.empty() ? format_->extension() : document.file_extension;
    file_output output(prefix_ + document.file_name + '.' + extension);

    auto last_match = document.text.c_str();
    // while we find standardese protocol URLs starting at last_match
    while (auto match = std::strstr(last_match, link_prefix))
    {
        // write from last_match to match
        output.write_str(last_match, match - last_match);

        // write correct URL
        auto entity_name = match + sizeof(link_prefix) - 1;
        auto end         = std::strchr(entity_name, '/');
        if (end == nullptr)
            end = &document.text.back() + 1;

        std::string name(entity_name, end - entity_name);
        auto        url = index_->get_linker().get_url(*index_, name, format_->extension());
        if (url.empty())
        {
            logger->warn("unable to resolve link to an entity named '{}'", name);
            last_match = entity_name + 1;
        }
        else
        {
            output.write_str(url.c_str(), url.size());
            last_match = entity_name + name.size() + 1;
        }
    }
    // write remainder of file
    output.write_str(last_match, &document.text.back() + 1 - last_match);
}
