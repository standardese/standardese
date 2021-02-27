// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/doc_section.hpp>

#include <standardese/markup/entity_kind.hpp>
#include <standardese/markup/list.hpp>

using namespace standardese::markup;

namespace
{
const char* get_suffix(section_type type) noexcept
{
    switch (type)
    {
    case section_type::brief:
        return "brief";
    case section_type::details:
        return "details";

    case section_type::requires:
        return "requires";
    case section_type::effects:
        return "effects";
    case section_type::synchronization:
        return "synchronization";
    case section_type::postconditions:
        return "postconditions";
    case section_type::returns:
        return "returns";
    case section_type::throws:
        return "throws";
    case section_type::complexity:
        return "complexity";
    case section_type::remarks:
        return "remarks";
    case section_type::error_conditions:
        return "error-conditions";
    case section_type::notes:
        return "notes";
    case section_type::preconditions:
        return "preconditions";
    case section_type::constraints:
        return "constraints";
    case section_type::diagnostics:
        return "diagnostics";

    case section_type::see:
        return "see";

    case section_type::count:
        break;
    }
    return "";
}

block_id get_section_id(type_safe::optional_ref<const entity> parent, section_type type)
{
    if (!parent
        || (parent.value().kind() != entity_kind::entity_documentation
            && parent.value().kind() != entity_kind::file_documentation))
        return block_id();
    auto entity_id = static_cast<const block_entity&>(parent.value()).id();
    return block_id(entity_id.as_str() + '-' + get_suffix(type));
}
} // namespace

block_id brief_section::id() const
{
    return get_section_id(parent(), section_type::brief);
}

entity_kind brief_section::do_get_kind() const noexcept
{
    return entity_kind::brief_section;
}

void brief_section::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> brief_section::do_clone() const
{
    builder b;
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<phrasing_entity>(child.clone()));
    return b.finish();
}

entity_kind details_section::do_get_kind() const noexcept
{
    return entity_kind::details_section;
}

void details_section::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> details_section::do_clone() const
{
    builder b;
    for (auto& child : *this)
        b.add_child(detail::unchecked_downcast<block_entity>(child.clone()));
    return b.finish();
}

entity_kind inline_section::do_get_kind() const noexcept
{
    return entity_kind::inline_section;
}

void inline_section::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *paragraph_)
        cb(mem, child);
}

std::unique_ptr<entity> inline_section::do_clone() const
{
    builder b(type_, name());
    for (auto& child : *paragraph_)
        b.add_child(detail::unchecked_downcast<phrasing_entity>(child.clone()));
    return b.finish();
}

entity_kind list_section::do_get_kind() const noexcept
{
    return entity_kind::list_section;
}

std::unique_ptr<list_section> list_section::build(std::string name,
                                                  std::unique_ptr<unordered_list> list)
{
    return std::unique_ptr<list_section>(new list_section(std::move(name), std::move(list)));
}

list_section::list_section(std::string name,
                           std::unique_ptr<unordered_list> list)
: name_(std::move(name)), list_(std::move(list))
{}

void list_section::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

std::unique_ptr<entity> list_section::do_clone() const
{
    return build(name(), detail::unchecked_downcast<unordered_list>(list_->clone()));
}
