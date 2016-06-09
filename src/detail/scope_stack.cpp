// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/scope_stack.hpp>

#include <cassert>

#include <standardese/cpp_class.hpp>
#include <standardese/cpp_enum.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

detail::scope_stack::scope_stack(cpp_file *file)
{
    stack_.emplace_back(file, cpp_cursor());
}

namespace
{
    bool is_container(const cpp_entity &e)
    {
        switch (e.get_entity_type())
        {
            case cpp_entity::file_t:
            case cpp_entity::namespace_t:
            case cpp_entity::enum_t:
            case cpp_entity::class_t:
            case cpp_entity::class_template_t:
            case cpp_entity::class_template_partial_specialization_t:
            case cpp_entity::class_template_full_specialization_t:
                return true;
            default:
                break;
        }

        return false;
    }

    void add_to_container(cpp_entity &container, cpp_entity_ptr ptr)
    {
        switch (container.get_entity_type())
        {
            case cpp_entity::file_t:
                static_cast<cpp_file&>(container).add_entity(std::move(ptr));
                break;

            case cpp_entity::namespace_t:
                static_cast<cpp_namespace&>(container).add_entity(std::move(ptr));
                break;

            case cpp_entity::enum_t:
                assert(ptr->get_entity_type() == cpp_entity::signed_enum_value_t
                    || ptr->get_entity_type() == cpp_entity::unsigned_enum_value_t);
                static_cast<cpp_enum&>(container).add_enum_value(detail::downcast<cpp_enum_value>(std::move(ptr)));
                break;

            case cpp_entity::class_t:
                static_cast<cpp_class&>(container).add_entity(std::move(ptr));
                break;
            case cpp_entity::class_template_t:
                static_cast<cpp_class_template&>(container).add_entity(std::move(ptr));
                break;
            case cpp_entity::class_template_full_specialization_t:
                static_cast<cpp_class_template_full_specialization&>(container).add_entity(std::move(ptr));
                break;
            case cpp_entity::class_template_partial_specialization_t:
                static_cast<cpp_class_template_partial_specialization&>(container).add_entity(std::move(ptr));
                break;

            default:
                assert(!"not a container");
                break;
        }
    }
}

bool detail::scope_stack::add_entity(cpp_entity_ptr ptr, cpp_cursor parent)
{
    auto entity = ptr.get();

    add_to_container(*stack_.back().first, std::move(ptr));

    if (is_container(*entity))
    {
        stack_.emplace_back(entity, parent);
        return true;
    }

    return false;
}

bool detail::scope_stack::pop_if_needed(cpp_cursor parent)
{
    for (auto iter = std::next(stack_.begin()); iter != stack_.end(); ++iter)
    {
        if (iter->second == parent)
        {
            stack_.erase(iter, stack_.end());
            return true;
        }
    }

    return false;
}
