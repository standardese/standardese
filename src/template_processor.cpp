// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/template_processor.hpp>

#include <algorithm>
#include <cstring>
#include <functional>
#include <vector>

#include <standardese/output.hpp>

using namespace standardese;

template_config::template_config() : delimiter_begin_("{{"), delimiter_end_("}}")
{
    set_command(template_command::generate_doc, "doc");
    set_command(template_command::generate_doc_text, "doc_text");
    set_command(template_command::generate_synopsis, "doc_synopsis");

    set_command(template_command::for_each, "for");
    set_command(template_command::end, "end");
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

    template <typename Func>
    std::string parse_commands(spdlog::logger& log, const template_config& config,
                               const char*& input, Func f)
    {
        std::string result;

        auto last_match = input;
        // while we find begin delimiter starting at last_match
        while (auto match = std::strstr(last_match, config.delimiter_begin().c_str()))
        {
            // append characters between matches
            result.append(last_match, match - last_match);

            // find end delimiter
            auto start = match + config.delimiter_begin().size();
            auto last  = std::strstr(start, config.delimiter_end().c_str());
            if (!last)
                break;
            auto end = last + config.delimiter_end().size();

            auto cur_command   = read_arg(start, last);
            auto prefix        = "standardese_";
            auto prefix_length = std::strlen(prefix);
            if (std::strncmp(cur_command.c_str(), prefix, prefix_length) == 0)
            {
                auto cmd = config.try_get_command(cur_command.c_str() + prefix_length);
                if (cmd != template_command::invalid)
                {
                    // process command
                    if (!f(result, cmd, start, last, end))
                    {
                        input = end;
                        return result;
                    }
                }
                else
                    log.warn("unknown template command '{}'", cur_command);
            }
            else
                // ignore command completely
                result.append(match, end - match);

            // set last match after the end location
            last_match = end;
        }

        // append characters between last match and end
        result.append(last_match);
        return result;
    }

    class variables
    {
    public:
        variables(const parser& p, const index& idx) STANDARDESE_NOEXCEPT : parser_(&p), idx_(&idx)
        {
        }

        void push(std::string name, const doc_entity& e)
        {
            vars_.emplace_back(std::move(name), &e);
        }

        const doc_entity* lookup(const std::string& name) const STANDARDESE_NOEXCEPT
        {
            for (auto iter = vars_.rbegin(); iter != vars_.rend(); ++iter)
                if (iter->first == name)
                    return iter->second;

            auto entity = idx_->try_lookup(name);
            if (!entity)
                parser_->get_logger()->warn("unable to find entity named '{}'", name);
            return entity;
        }

        bool pop_scope()
        {
            if (vars_.empty())
            {
                parser_->get_logger()->warn("end in top-level scope");
                return false;
            }
            vars_.pop_back();
            return true;
        }

        const parser& get_parser() const STANDARDESE_NOEXCEPT
        {
            return *parser_;
        }

    private:
        std::vector<std::pair<std::string, const doc_entity*>> vars_;
        const parser* parser_;
        const index*  idx_;
    };

    md_ptr<md_document> get_documentation(const variables& vars, const std::string& name)
    {
        auto entity = vars.lookup(name);
        if (!entity)
            return nullptr;

        auto doc = md_document::make("");
        entity->generate_documentation(vars.get_parser(), *doc);
        return doc;
    }

    md_ptr<md_document> get_synopsis(const variables& vars, const std::string& name)
    {
        auto entity = vars.lookup(name);
        if (!entity)
            return nullptr;

        auto              doc = md_document::make("");
        code_block_writer writer(*doc);
        entity->generate_synopsis(vars.get_parser(), writer);
        doc->add_entity(writer.get_code_block());

        return doc;
    }

    md_ptr<md_document> get_documentation_text(const variables& vars, const std::string& name)
    {
        auto entity = vars.lookup(name);
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
    variables vars(p, i);
    std::function<bool(std::string&, template_command, const char*, const char*, const char*&)>
        func = [&](std::string& result, template_command cur_command, const char* ptr,
                   const char* last, const char*& end) {
            switch (cur_command)
            {
            case template_command::generate_doc:
                if (auto doc = get_documentation(vars, read_arg(ptr, last)))
                    result += write_document(p, i, std::move(doc), read_arg(ptr, last));
                break;
            case template_command::generate_synopsis:
                if (auto doc = get_synopsis(vars, read_arg(ptr, last)))
                    result += write_document(p, i, std::move(doc), read_arg(ptr, last));
                break;
            case template_command::generate_doc_text:
                if (auto doc = get_documentation_text(vars, read_arg(ptr, last)))
                    result += write_document(p, i, std::move(doc), read_arg(ptr, last));
                break;

            case template_command::for_each:
            {
                auto var_name = read_arg(ptr, last);
                auto entity   = vars.lookup(read_arg(ptr, last));
                if (!entity)
                    break;
                else if (entity->begin() != entity->end())
                {
                    for (auto& child : *entity)
                    {
                        vars.push(var_name, child);

                        // parse starting after the for loop
                        auto begin = last + config.delimiter_end().size();

                        auto res = parse_commands(*p.get_logger(), config, begin, func);
                        result += res;

                        // begin now points to after the for loop
                        // start in this handler after loop as well
                        end = begin;
                    }
                }
                else
                {
                    // entity is empty, skip until we've reached the end of the for loop
                    // hacky solution: set variable to current entity,
                    // process everything and discard it
                    vars.push(var_name, *entity);

                    auto begin = last + config.delimiter_end().size();
                    parse_commands(*p.get_logger(), config, begin, func);
                    end = begin;
                }
                break;
            }
            case template_command::end:
                return !vars.pop_scope();

            case template_command::invalid:
                assert(false);
            }
            return true;
        };
    auto ptr = input.c_str();
    return parse_commands(*p.get_logger(), config, ptr, func);
}
