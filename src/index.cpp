// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/index.hpp>

#include <cctype>
#include <spdlog/details/format.h>

#include <standardese/comment.hpp>

using namespace standardese;

namespace
{
    std::string get_id(const std::string& unique_name)
    {
        std::string result;
        for (auto c : unique_name)
            if (std::isspace(c))
                continue;
            else
                result += c;
        return result;
    }
}

void index::register_comment(const md_comment& comment) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto                        id    = get_id(comment.get_unique_name());
    auto                        first = comments_.emplace(std::move(id), &comment).second;
    if (!first)
        throw std::logic_error("multiple comments for one entity");
}

const md_comment* index::try_lookup(const std::string& unique_name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto                        iter = comments_.find(get_id(unique_name));
    return iter == comments_.end() ? nullptr : iter->second;
}

const md_comment& index::lookup(const std::string& unique_name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return *comments_.at(get_id(unique_name));
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
    auto comment = try_lookup(unique_name);
    if (!comment)
    {
        for (auto& pair : external_)
            if (matches(pair.first, unique_name))
                return generate(pair.second, unique_name);
        return "";
    }
    return comment->get_output_name() + '.' + extension + '#' + comment->get_unique_name();
}
