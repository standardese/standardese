// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
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

    std::string fragment_encode(const char* url, bool external = false)
    {
        std::string result;

        while (*url)
        {
            auto c = *url++;
            if (is_valid_fragment(c))
                result += c;
            else if (!external && (c == '<' || c == '>'))
                result += '-';
            else if (c == ' ')
                ; // ignore
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
                result += fragment_encode(unique_name, true);
            else
                result += *iter;
        }

        return result;
    }

    const doc_entity& get_documented_entity(const doc_entity& e)
    {
        auto result = &e;
        if (e.has_comment() && e.get_comment().in_member_group())
        {
            assert(e.has_parent()
                   && e.get_parent().get_entity_type() == doc_entity::member_group_t);
            result = &*static_cast<const doc_member_group&>(e.get_parent()).begin();
        }
        else
        {
            while (result->has_parent()
                   && (!result->has_comment() || result->get_comment().empty()))
                result = &result->get_parent();
            assert(result);
        }

        return *result;
    }
}

void external_linker::register_external(std::string prefix, std::string url)
{
    auto res          = external_.emplace(std::move(prefix), "");
    res.first->second = std::move(url);
}

std::string external_linker::lookup(const std::string& unique_name) const
{
    for (auto& pair : external_)
        if (matches(pair.first, unique_name.c_str()))
            return generate(pair.second, unique_name.c_str());
    return "";
}

void linker::register_entity(const doc_entity& e, std::string output_file) const
{
    auto loc = location(get_documented_entity(e), "doc_" + std::move(output_file));

    std::unique_lock<std::mutex> lock(mutex_);
    auto                         res = locations_.emplace(&e, std::move(loc));
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

std::string linker::get_url(const index& idx, const external_linker& external,
                            const doc_entity* context, const std::string& unique_name,
                            const char* extension) const
{
    auto entity =
        context ? idx.try_name_lookup(*context, unique_name) : idx.try_lookup(unique_name);
    if (entity && entity->get_cpp_entity_type() != cpp_entity::namespace_t)
        // use anchor for namespace
        return get_url(*entity, extension);

    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto                         iter = anchors_.find(unique_name);
        if (iter != anchors_.end())
            return iter->second.format(extension);
    }

    return external.lookup(unique_name);
}

std::string linker::get_url(const doc_entity& e, const char* extension) const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return locations_.at(&e).format(extension);
}

std::string linker::get_anchor_id(const doc_entity& e) const
{
    return fragment_encode(e.get_unique_name().c_str());
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
