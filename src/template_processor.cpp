// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/template_processor.hpp>

#include <algorithm>
#include <cstring>

#include <standardese/output.hpp>

using namespace standardese;

template_config::template_config() : delimiter_begin_("{{"), delimiter_end_("}}")
{
    set_command(template_command::generate_doc, "standardese_doc");
    set_command(template_command::generate_doc_text, "standardese_doc_text");
    set_command(template_command::generate_synopsis, "standardese_doc_synopsis");
}

void template_config::set_command(template_command cmd, std::string str)
{
    commands_[static_cast<std::size_t>(cmd)] = std::move(str);
}

template_command template_config::get_command(const std::string& str) const
{
    auto res = try_get_command(str);
    if (res == template_command::invalid)
        throw std::logic_error(fmt::format("invalid template command '{}'", str));
    return res;
}

template_command template_config::try_get_command(const std::string& str) const STANDARDESE_NOEXCEPT
{
    auto iter = std::find(std::begin(commands_), std::end(commands_), str);
    if (iter == std::end(commands_))
        return template_command::invalid;
    return static_cast<template_command>(iter - std::begin(commands_));
}

namespace
{
    using standardese::index;

    std::string read_arg(const char*& ptr, const char* end)
    {
        while (ptr != end && std::isspace(*ptr))
            ++ptr;

        std::string result;
        while (ptr != end && !std::isspace(*ptr))
            result += *ptr++;

        return result;
    }

    const doc_entity* lookup(const parser& p, const index& i, const std::string& unique_name)
    {
        auto entity = i.try_lookup(unique_name);
        if (!entity)
            p.get_logger()->warn("unable to find entity named '{}'", unique_name);
        return entity;
    }

    md_ptr<md_document> get_documentation(const parser& p, const index& i,
                                          const std::string& unique_name)
    {
        auto entity = lookup(p, i, unique_name);
        if (!entity)
            return nullptr;

        auto doc = md_document::make("");
        entity->generate_documentation(p, *doc);
        return doc;
    }

    md_ptr<md_document> get_synopsis(const parser& p, const index& i,
                                     const std::string& unique_name)
    {
        auto entity = lookup(p, i, unique_name);
        if (!entity)
            return nullptr;

        auto              doc = md_document::make("");
        code_block_writer writer(*doc);
        entity->generate_synopsis(p, writer);
        doc->add_entity(writer.get_code_block());

        return doc;
    }

    md_ptr<md_document> get_documentation_text(const parser& p, const index& i,
                                               const std::string& unique_name)
    {
        auto entity = lookup(p, i, unique_name);
        if (!entity)
            return nullptr;

        auto doc = md_document::make("");
        doc->add_entity(entity->get_comment().get_content().clone(*doc));

        return doc;
    }

    std::string write_document(const parser& p, const index& i, md_ptr<md_document> doc,
                               const std::string& format_name)
    {
        auto          format = make_output_format(format_name);
        string_output output;

        resolve_urls(p.get_logger(), i, *doc, format->extension());
        format->render(output, *doc);

        return output.get_string();
    }
}

std::string standardese::process_template(const parser& p, const index& i,
                                          const template_config& config, const std::string& input)
{
    std::string result;

    auto last_match = input.c_str();
    // while we find begin delimiter starting at last_match
    while (auto match = std::strstr(last_match, config.delimiter_begin().c_str()))
    {
        // append characters between matches
        result.append(last_match, match - last_match);

        // find end delimiter
        match += config.delimiter_begin().size();
        auto end = std::strstr(match, config.delimiter_end().c_str());
        if (!end)
            break;

        // process command
        auto cur_command = read_arg(match, end);
        switch (config.try_get_command(cur_command))
        {
        case template_command::generate_doc:
            if (auto doc = get_documentation(p, i, read_arg(match, end)))
                result += write_document(p, i, std::move(doc), read_arg(match, end));
            break;
        case template_command::generate_synopsis:
            if (auto doc = get_synopsis(p, i, read_arg(match, end)))
                result += write_document(p, i, std::move(doc), read_arg(match, end));
            break;
        case template_command::generate_doc_text:
            if (auto doc = get_documentation_text(p, i, read_arg(match, end)))
                result += write_document(p, i, std::move(doc), read_arg(match, end));
            break;
        case template_command::invalid:
            p.get_logger()->warn("unknown template command '{}'", cur_command);
            break;
        }

        // set match to after end delimiter
        // set last match to the same location
        // (match will be updated to the next delimiter start)
        match      = end + config.delimiter_end().size();
        last_match = match;
    }

    // append characters between last match and end
    result.append(last_match);

    return result;
}
