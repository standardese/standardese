// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/doc_entity.hpp>

#include <standardese/parser.hpp>

using namespace standardese;

doc_entity::doc_entity(const parser& p, const cpp_entity& entity, string output_name)
: output_name_(std::move(output_name)),
  entity_(&entity),
  comment_(p.get_comment_registry().lookup_comment(p.get_entity_registry(), entity))
{
    if (entity.get_entity_type() == cpp_entity::file_t && comment_
        && comment_->has_unique_name_override())
        output_name_ = comment_->get_unique_name_override();
}

cpp_entity::type doc_entity::get_entity_type() const
{
    return get_cpp_entity().get_entity_type();
}

cpp_name doc_entity::get_unique_name() const
{
    if (has_comment() && get_comment().has_unique_name_override())
        return get_comment().get_unique_name_override();
    return entity_->get_unique_name();
}
