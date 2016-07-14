// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_INDEX_HPP_INCLUDED
#define STANDARDESE_INDEX_HPP_INCLUDED

#include <mutex>
#include <unordered_map>

namespace standardese
{
    class md_comment;

    class index
    {
    public:
        void register_comment(const std::string& unique_name, const md_comment& comment) const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto                        id    = get_id(unique_name);
            auto                        first = comments_.emplace(std::move(id), &comment).second;
            if (!first)
                throw std::logic_error("multiple comments for one entity");
        }

        const md_comment* try_lookup(const std::string& unique_name) const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto                        iter = comments_.find(get_id(unique_name));
            return iter == comments_.end() ? nullptr : iter->second;
        }

        const md_comment& lookup(const std::string& unique_name) const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return *comments_.at(get_id(unique_name));
        }

    private:
        std::string get_id(const std::string& unique_name) const
        {
            std::string result;
            for (auto c : unique_name)
                if (std::isspace(c))
                    continue;
                else
                    result += c;
            return result;
        }

        mutable std::mutex mutex_;
        mutable std::unordered_map<std::string, const md_comment*> comments_;
    };
} // namespace standardese

#endif // STANDARDESE_INDEX_HPP_INCLUDED
