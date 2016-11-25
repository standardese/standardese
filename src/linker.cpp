// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/linker.hpp>

#include <spdlog/fmt/fmt.h>

#include <standardese/comment.hpp>
#include <standardese/doc_entity.hpp>
#include <standardese/index.hpp>

using namespace standardese;

namespace
{
    bool is_valid_fragment(char c)
    {
        if (std::isalnum(c))
            return c;
        static auto allowed = "!$&'()*+,;=-._~:@/?";
        return std::strchr(allowed, c) != nullptr;
    }

    std::string fragment_encode(const char* url)
    {
        std::string result;

        while (*url)
        {
            auto c = *url++;
            if (is_valid_fragment(c))
                result += c;
            else if (c == '<' || c == '>')
                result += '-';
            else
            {
                // escape character
                result += '%';
                result += fmt::format("{0:x}", int(c));
            }
        }

        return result;
    }

    bool matches(const std::string& prefix, const char* unique_name)
    {
        return std::strncmp(unique_name, prefix.c_str(), prefix.size()) == 0;
    }

    std::string generate(const std::string& url, const char* unique_name)
    {
        std::string result;
        for (auto iter = url.begin(); iter != url.end(); ++iter)
        {
            if (*iter == '$' && iter != std::prev(url.end()) && *++iter == '$')
                // sequence of two dollar signs
                result += fragment_encode(unique_name);
            else
                result += *iter;
        }

        return result;
    }
}

void linker::register_external(std::string prefix, std::string url)
{
    auto res          = external_.emplace(std::move(prefix), "");
    res.first->second = std::move(url);
}

void linker::register_entity(const doc_entity& e, std::string output_file) const
{
    auto entity = &e;
    if (entity->has_comment() && entity->get_comment().in_member_group())
    {
        assert(entity->has_parent()
               && entity->get_parent().get_entity_type() == doc_entity::member_group_t);
        entity = &entity->get_parent();
    }

    std::unique_lock<std::mutex> lock(mutex_);
    auto res = locations_.emplace(&e, location(*entity, std::move(output_file)));
    if (!res.second)
        throw std::logic_error(fmt::format("linker: duplicate registration of entity '{}'",
                                           e.get_unique_name().c_str()));
}

std::string linker::register_anchor(const std::string& unique_name, std::string output_file) const
{
    location loc(unique_name.c_str(), std::move(output_file));
    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto                         res = anchors_.emplace(unique_name, loc);
        if (!res.second)
            res.first->second = loc;
    }
    return loc.get_id();
}

void linker::change_output_file(const doc_entity& e, std::string output_file) const
{
    std::unique_lock<std::mutex> lock(mutex_);
    locations_.at(&e).set_output_file(std::move(output_file));
}

namespace
{
    using standardese::index;

    const doc_entity* lookup_name(const index& idx, const doc_entity* context,
                                  const std::string& unique_name)
    {
        if (unique_name.front() == '.' && context)
        {
            for (auto cur = context; cur; cur = cur->has_parent() ? &cur->get_parent() : nullptr)
            {
                auto name =
                    std::string(cur->get_unique_name().c_str()) + "::" + (unique_name.c_str() + 1);
                if (auto entity = idx.try_lookup(name))
                    return entity;
            }
        }

        return idx.try_lookup(unique_name);
    }
}

std::string linker::get_url(const index& idx, const doc_entity* context,
                            const std::string& unique_name, const char* extension) const
{
    auto entity = lookup_name(idx, context, unique_name);
    if (entity)
        return get_url(*entity, extension);

    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto                         iter = anchors_.find(unique_name);
        if (iter != anchors_.end())
            return iter->second.format(extension);
    }

    for (auto& pair : external_)
        if (matches(pair.first, unique_name.c_str()))
            return generate(pair.second, unique_name.c_str());
    return "";
}

std::string linker::get_url(const doc_entity& e, const char* extension) const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return locations_.at(&e).format(extension);
}

std::string linker::get_anchor_id(const doc_entity& e) const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return locations_.at(&e).get_id();
}

md_ptr<md_anchor> linker::get_anchor(const doc_entity& e, const md_entity& parent) const
{
    return md_anchor::make(parent, get_anchor_id(e).c_str());
}

linker::location::location(const doc_entity& e, std::string output_file)
: location(e.get_unique_name().c_str(), std::move(output_file))
{
}

linker::location::location(const char* unique_name, std::string output_file)
: id_(fragment_encode(unique_name))
{
    set_output_file(std::move(output_file));
}

void linker::location::set_output_file(std::string output_file)
{
    file_name_      = std::move(output_file);
    with_extension_ = false;
    for (auto iter = file_name_.rbegin(); iter != file_name_.rend(); ++iter)
        if (*iter == '.')
        {
            with_extension_ = true;
            break;
        }
}

std::string linker::location::format(const char* extension) const
{
    auto result = file_name_;

    if (!with_extension_)
    {
        result += '.';
        result += extension;
    }

    if (!id_.empty())
    {
        result += '#';
        result += id_;
    }

    return result;
}
