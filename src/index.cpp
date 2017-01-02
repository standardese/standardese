// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/index.hpp>

#include <algorithm>
#include <cctype>
#include <spdlog/fmt/fmt.h>

#include <standardese/comment.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

std::string detail::get_id(const std::string& unique_name)
{
    std::string result;
    for (auto c : unique_name)
        if (std::isspace(c))
            continue;
        else
            result += c;

    if (result.size() >= 2 && result.end()[-1] == ')' && result.end()[-2] == '(')
    {
        // ends with ()
        result.pop_back();
        result.pop_back();
    }

    return result;
}

std::string detail::get_short_id(const std::string& id)
{
    std::string result;

    auto skip = false;
    for (auto ptr = id.c_str(); *ptr; ++ptr)
    {
        auto c = *ptr;
        if (c == '(')
            skip = true;
        else if (c == '.')
        {
            if (ptr[1] == '.')
            {
                // ... token
                assert(ptr[2] == '.');
                ptr += 2; // skip token as well
            }
            else
            {
                // next token is '.' separator for parameter
                result += '.';
                skip = false;
            }
        }
        else if (c == '<')
            skip = true;
        else if (c == '>')
            skip = false;
        else if (!skip)
            result += c;
    }

    return result;
}

void index::register_entity(const parser& p, const doc_entity& entity,
                            std::string output_file) const
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
            auto res = entities_.emplace(std::move(short_id), std::make_pair(true, &entity)).second;
            assert(res);
            (void)res;
        }
        else if (iter->second.first)
            // it was a short name
            entities_.erase(iter);
    }

    // insert long id
    auto pair = entities_.emplace(std::move(id), std::make_pair(false, &entity));
    if (!pair.second && entity.get_cpp_entity_type() != cpp_entity::namespace_t)
        p.get_logger()->warn("duplicate index registration of an entity named '{}'",
                             entity.get_unique_name().c_str());
    else if (pair.first->second.second->get_cpp_entity_type() == cpp_entity::file_t)
    {
        using value_type = decltype(files_)::value_type;
        auto pos         = std::lower_bound(files_.begin(), files_.end(), pair.first,
                                    [](value_type a, value_type b) { return a->first < b->first; });
        files_.insert(pos, pair.first);
    }

    if (entity.in_module())
    {
        auto pos = std::lower_bound(modules_.begin(), modules_.end(), entity.get_module());
        modules_.insert(pos, entity.get_module());
    }

    linker_.register_entity(entity, std::move(output_file));
}

const doc_entity* index::try_lookup(const std::string& unique_name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto                        iter = entities_.find(detail::get_id(unique_name));
    return iter == entities_.end() ? nullptr : iter->second.second;
}

const doc_entity& index::lookup(const std::string& unique_name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return *entities_.at(detail::get_id(unique_name)).second;
}

const doc_entity* index::try_name_lookup(const doc_entity&  context,
                                         const std::string& unique_name) const
{
    if (unique_name.front() == '?' || unique_name.front() == '*')
    {
        // first try parameter/base
        auto name =
            std::string(context.get_unique_name().c_str()) + "." + (unique_name.c_str() + 1);
        if (auto entity = try_lookup(name))
            return entity;

        // then look for other names
        for (auto cur = &context; cur; cur = cur->has_parent() ? &cur->get_parent() : nullptr)
        {
            auto name =
                std::string(cur->get_unique_name().c_str()) + "::" + (unique_name.c_str() + 1);
            if (auto entity = try_lookup(name))
                return entity;
        }
    }

    return try_lookup(unique_name);
}

const doc_entity& index::name_lookup(const doc_entity&  context,
                                     const std::string& unique_name) const
{
    auto result = try_name_lookup(context, unique_name);
    if (!result)
        throw std::invalid_argument(fmt::format("unable to find entity named '{}'", unique_name));
    return *result;
}

void index::namespace_member_impl(ns_member_cb cb, void* data)
{
    for (auto& pair : entities_)
    {
        auto& value = pair.second;
        if (value.first)
            continue; // ignore short names

        auto& entity = *value.second;
        if (entity.get_cpp_entity_type() == cpp_entity::namespace_t
            || entity.get_cpp_entity_type() == cpp_entity::file_t)
            continue;

        assert(entity.has_parent());
        auto* parent = &entity.get_parent();
        if (parent->get_entity_type() == doc_entity::member_group_t)
        {
            assert(parent->has_parent());
            parent = &parent->get_parent();
        }

        auto parent_type = parent->get_cpp_entity_type();
        if (parent_type == cpp_entity::namespace_t)
            cb(parent, entity, data);
        else if (parent_type == cpp_entity::file_t)
            cb(nullptr, entity, data);
    }
}
