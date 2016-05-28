// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_SYNOPSIS_HPP_INCLUDED
#define STANDARDESE_SYNOPSIS_HPP_INCLUDED

#include <bitset>
#include <set>

#include <standardese/cpp_entity.hpp>
#include <standardese/output.hpp>

namespace standardese
{
    class entity_blacklist
    {
    public:
        static const struct empty_t
        {
            empty_t() {}
        } empty;

        entity_blacklist(empty_t) {}

        entity_blacklist();

        static const struct documentation_t
        {
            documentation_t() {}
        } documentation;

        static const struct synopsis_t
        {
            synopsis_t() {}
        } synopsis;

        void blacklist(documentation_t, const cpp_name &name, cpp_entity::type type = cpp_entity::invalid_t)
        {
            doc_blacklist_.emplace(name, type);
        }

        void blacklist(synopsis_t, const cpp_name &name, cpp_entity::type type = cpp_entity::invalid_t)
        {
            synopsis_blacklist_.emplace(name, type);
        }

        void blacklist(const cpp_name &name, cpp_entity::type type = cpp_entity::invalid_t)
        {
            blacklist(documentation, name, type);
            blacklist(synopsis, name, type);
        }

        void blacklist(documentation_t, cpp_entity::type type)
        {
            type_blacklist_.set(type);
        }

        bool is_blacklisted(documentation_t, const cpp_entity &e) const;

        bool is_blacklisted(synopsis_t, const cpp_entity &e) const;

    private:
        std::set<std::pair<cpp_name, cpp_entity::type>> doc_blacklist_, synopsis_blacklist_;
        std::bitset<cpp_entity::invalid_t> type_blacklist_;
    };

    void write_synopsis(output_base &out, const cpp_entity &e, const entity_blacklist &blacklist);
} // namespace standardese

#endif // STANDARDESE_SYNOPSIS_HPP_INCLUDED
