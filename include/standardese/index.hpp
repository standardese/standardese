// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_INDEX_HPP_INCLUDED
#define STANDARDESE_INDEX_HPP_INCLUDED

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <standardese/doc_entity.hpp>
#include <standardese/translation_unit.hpp>

namespace standardese
{
    namespace detail
    {
        std::string get_id(const std::string& unique_name);

        // returns the short id
        // it doesn't require parameters
        std::string get_short_id(const std::string& id);
    } // namespace detail

    class cpp_namespace;

    class index
    {
    public:
        void register_entity(const doc_entity& entity) const;

        const doc_entity* try_lookup(const std::string& unique_name) const;

        const doc_entity& lookup(const std::string& unique_name) const;

        // void(const doc_entity&)
        template <typename Func>
        void for_each_file(Func f)
        {
            for (auto& iter : files_)
                f(*iter->second.second);
        }

        // void(const doc_entity* ns, const doc_entity& member)
        template <typename Func>
        void for_each_namespace_member(Func f)
        {
            auto cb = [](const doc_entity* ns, const doc_entity& e, void* data) {
                auto& func = *static_cast<Func*>(data);
                func(ns, e);
            };
            namespace_member_impl(cb, &f);
        }

        // void(const std::string&)
        template <typename Func>
        void for_each_module(Func f)
        {
            for (auto& m : modules_)
                f(m);
        }

    private:
        using ns_member_cb = void(const doc_entity*, const doc_entity&, void*);

        void namespace_member_impl(ns_member_cb cb, void* data);

        mutable std::mutex mutex_;
        mutable std::map<std::string, std::pair<bool, const doc_entity*>> entities_;
        mutable std::vector<decltype(entities_)::const_iterator> files_;
        mutable std::vector<std::string>                         modules_;
    };
} // namespace standardese

#endif // STANDARDESE_INDEX_HPP_INCLUDED
