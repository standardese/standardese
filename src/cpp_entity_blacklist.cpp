// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_entity_blacklist.hpp>

#include <standardese/detail/synopsis_utils.hpp>
#include <standardese/cpp_class.hpp>
#include <standardese/cpp_enum.hpp>
#include <standardese/cpp_function.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/cpp_type.hpp>
#include <standardese/cpp_variable.hpp>
#include <standardese/doc_entity.hpp>
#include <standardese/md_custom.hpp>
#include <standardese/output.hpp>
#include <standardese/parser.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

const entity_blacklist::empty_t         entity_blacklist::empty;
const entity_blacklist::synopsis_t      entity_blacklist::synopsis;
const entity_blacklist::documentation_t entity_blacklist::documentation;

entity_blacklist::entity_blacklist()
{
    type_blacklist_.set(cpp_entity::inclusion_directive_t);
    type_blacklist_.set(cpp_entity::using_declaration_t);
    type_blacklist_.set(cpp_entity::using_directive_t);
    type_blacklist_.set(cpp_entity::access_specifier_t);
}

namespace
{
    bool is_blacklisted(const std::set<std::pair<cpp_name, cpp_entity::type>>& blacklist,
                        const cpp_name& name, cpp_entity::type t)
    {
        // set is firsted sorted by name, then type
        // so this is the first pair with the name
        static_assert(int(cpp_entity::file_t) == 0, "file must come first");
        auto first = std::make_pair(name, cpp_entity::file_t);

        auto iter = blacklist.lower_bound(first);
        if (iter == blacklist.end() || iter->first != name)
            // no element with this name found
            return false;

        // iterate until name doesn't match any more
        for (; iter != blacklist.end() && iter->first == name; ++iter)
            if (iter->second == cpp_entity::invalid_t)
                // match all
                return true;
            else if (iter->second == t)
                return true;

        return false;
    }
}

bool entity_blacklist::is_blacklisted(documentation_t, const cpp_entity& e) const
{
    if (type_blacklist_[e.get_entity_type()])
        return true;
    return ::is_blacklisted(doc_blacklist_, e.get_name(), e.get_entity_type());
}

bool entity_blacklist::is_blacklisted(synopsis_t, const doc_entity& e) const
{
    for (auto cur = &e; cur; cur = cur->has_parent() ? &cur->get_parent() : nullptr)
        if (::is_blacklisted(synopsis_blacklist_, cur->get_name(), cur->get_cpp_entity_type()))
            return true;

    return false;
}

bool entity_blacklist::is_blacklisted(synopsis_t, const cpp_entity& e) const
{
    for (auto cur = &e; cur; cur = cur->get_semantic_parent())
        if (::is_blacklisted(synopsis_blacklist_, cur->get_name(), cur->get_entity_type()))
            return true;

    return false;
}
