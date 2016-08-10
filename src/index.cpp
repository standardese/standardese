// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/index.hpp>

#include <algorithm>
#include <cctype>
#include <spdlog/fmt/fmt.h>

#include <standardese/comment.hpp>
#include <standardese/cpp_namespace.hpp>

using namespace standardese;

std::string detail::get_id(const std::string& unique_name)
{
    std::string result;
    for (auto c : unique_name)
        if (std::isspace(c))
            continue;
        else
            result += c;

    if (result.end()[-1] == ')' && result.end()[-2] == '(')
    {
        // ends with ()
        result.pop_back();
        result.pop_back();
    }

    return result;
}

std::string detail::get_short_id(const std::string& id)
{
    auto open_paren = id.find('(');
    if (open_paren == std::string::npos)
        return id;
    auto close_paren = id.rfind(')');
    assert(id.at(close_paren) == ')');

    return id.substr(0, open_paren) + id.substr(close_paren + 1);
}

void index::register_entity(doc_entity entity) const
{
    auto id       = detail::get_id(entity.get_unique_name().c_str());
    auto short_id = detail::get_short_id(id);

    std::lock_guard<std::mutex> lock(mutex_);

    // insert short id if it doesn't exist
    // otherwise erase
    if (short_id != id)
    {
        auto iter = entities_.find(short_id);
        if (iter == entities_.end())
        {
            auto res = entities_.emplace(std::move(short_id), std::make_pair(true, entity)).second;
            assert(res);
        }
        else if (iter->second.first)
            // it was a short name
            entities_.erase(iter);
    }

    // insert long id
    auto pair = entities_.emplace(std::move(id), std::make_pair(false, entity));
    if (!pair.second)
        throw std::logic_error(fmt::format("duplicate index registration of an entity named '{}'",
                                           entity.get_unique_name().c_str()));
    else if (pair.first->second.second.get_entity_type() == cpp_entity::file_t)
    {
        using value_type = decltype(files_)::value_type;
        auto pos         = std::lower_bound(files_.begin(), files_.end(), pair.first,
                                    [](value_type a, value_type b) { return a->first < b->first; });
        files_.insert(pos, pair.first);
    }
}

const doc_entity* index::try_lookup(const std::string& unique_name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto                        iter = entities_.find(detail::get_id(unique_name));
    return iter == entities_.end() ? nullptr : &iter->second.second;
}

const doc_entity& index::lookup(const std::string& unique_name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return entities_.at(detail::get_id(unique_name)).second;
}

namespace
{
    bool matches(const std::string& prefix, const std::string& unique_name)
    {
        return unique_name.compare(0, prefix.size(), prefix) == 0;
    }

    bool is_valid_url(char c)
    {
        return std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~';
    }

    std::string url_encode(const std::string& url)
    {
        std::string result;

        for (auto c : url)
        {
            if (is_valid_url(c))
                result += c;
            else
            {
                // escape character
                result += '%';
                result += fmt::format("{0:x}", int(c));
            }
        }

        return result;
    }

    std::string generate(const std::string& url, const std::string& unique_name)
    {
        std::string result;
        for (auto iter = url.begin(); iter != url.end(); ++iter)
        {
            if (*iter == '$' && iter != std::prev(url.end()) && *++iter == '$')
                // sequence of two dollar signs
                result += url_encode(unique_name);
            else
                result += *iter;
        }

        return result;
    }
}

std::string index::get_url(const std::string& unique_name, const char* extension) const
{
    auto entity = try_lookup(unique_name);
    if (!entity)
    {
        for (auto& pair : external_)
            if (matches(pair.first, unique_name))
                return generate(pair.second, unique_name);
        return "";
    }

    if (entity->get_entity_type() == cpp_entity::file_t)
        return fmt::format("{}.{}", entity->get_output_name().c_str(), extension);
    else
        return fmt::format("{}.{}#{}", entity->get_output_name().c_str(), extension,
                           entity->get_unique_name().c_str());
}

void index::namespace_member_impl(ns_member_cb cb, void* data)
{
    for (auto& pair : entities_)
    {
        auto& value = pair.second;
        if (value.first)
            continue; // ignore short names
        auto& entity = value.second.get_cpp_entity();
        if (entity.get_entity_type() == cpp_entity::namespace_t
            || entity.get_entity_type() == cpp_entity::file_t)
            continue;

        assert(entity.has_parent());
        auto& parent      = entity.get_parent();
        auto  parent_type = parent.get_entity_type();
        if (parent_type == cpp_entity::namespace_t)
            cb(static_cast<const cpp_namespace*>(&parent), value.second, data);
        else if (parent_type == cpp_entity::file_t)
            cb(nullptr, value.second, data);
    }
}
