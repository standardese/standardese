// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_INDEX_HPP_INCLUDED
#define STANDARDESE_INDEX_HPP_INCLUDED

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <standardese/doc_entity.hpp>
#include <standardese/translation_unit.hpp>

namespace standardese
{
    class cpp_namespace;

    class index
    {
    public:
        /// \effects Registers an external URL.
        /// All unresolved `unique-name`s starting with `prefix` will be resolved to `url`.
        /// If `url` contains two dollar signs (`$$`), this will be replaced by the (url-encoded) `unique-name`.
        void register_external(std::string prefix, std::string url)
        {
            auto iter = external_.find(prefix);
            if (iter == external_.end())
                external_.emplace(std::move(prefix), std::move(url));
            else
                iter->second = std::move(url);
        }

        void register_entity(doc_entity entity) const;

        const doc_entity* try_lookup(const std::string& unique_name) const;

        const doc_entity& lookup(const std::string& unique_name) const;

        std::string get_url(const std::string& unique_name, const char* extension) const;

        // void(const cpp_file&)
        template <typename Func>
        void for_each_file(Func f)
        {
            for (auto iter : files_)
                f(static_cast<const cpp_file&>(iter->second.second.get_cpp_entity()));
        }

        // void(const cpp_namespace* ns, const cpp_entity& member)
        template <typename Func>
        void for_each_namespace_member(Func f)
        {
            auto cb = [](const cpp_namespace* ns, const cpp_entity& e, void* data) {
                auto& func = *static_cast<Func*>(data);
                func(ns, e);
            };
            namespace_member_impl(cb, &f);
        }

    private:
        using ns_member_cb = void(const cpp_namespace*, const cpp_entity&, void*);

        void namespace_member_impl(ns_member_cb cb, void* data);

        mutable std::mutex mutex_;
        mutable std::unordered_map<std::string, std::pair<bool, doc_entity>> entities_;
        mutable std::vector<decltype(entities_)::const_iterator> files_;

        std::unordered_map<std::string, std::string> external_;
    };
} // namespace standardese

#endif // STANDARDESE_INDEX_HPP_INCLUDED
