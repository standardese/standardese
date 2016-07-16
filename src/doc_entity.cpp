// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/doc_entity.hpp>

#include <standardese/translation_unit.hpp>

using namespace standardese;

doc_entity::doc_entity(const cpp_entity& entity)
: entity_(&entity), comment_(entity.has_comment() ? &entity.get_comment() : nullptr)
{
}

doc_entity::doc_entity(const cpp_entity& entity, const md_comment& comment)
: entity_(&entity), comment_(&comment)
{
}

cpp_entity::type doc_entity::get_entity_type() const
{
    return get_cpp_entity().get_entity_type();
}

cpp_name doc_entity::get_unique_name() const
{
    if (has_comment() && get_comment().has_unique_name())
        return get_comment().get_unique_name();
    return entity_->get_unique_name();
}

cpp_name doc_entity::get_output_name() const
{
    if (get_entity_type() == cpp_entity::file_t)
        return static_cast<const cpp_file&>(get_cpp_entity()).get_output_name();
    assert(has_comment());
    return get_comment().get_output_name();
}