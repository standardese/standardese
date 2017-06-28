// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/document.hpp>

#include <fstream>

#include <standardese/markup/entity_kind.hpp>

using namespace standardese::markup;

void document_entity::do_visit(detail::visitor_callback_t cb, void* mem) const
{
    for (auto& child : *this)
        cb(mem, child);
}

entity_kind main_document::do_get_kind() const noexcept
{
    return entity_kind::main_document;
}

entity_kind subdocument::do_get_kind() const noexcept
{
    return entity_kind::subdocument;
}

namespace
{
    output_name get_output_name(std::string file_name)
    {
        auto pos = file_name.find('.');
        if (pos == std::string::npos)
            return output_name::from_name(std::move(file_name));
        return output_name::from_file_name(std::move(file_name));
    }
}

template_document::template_document(std::string title, std::string file_name)
: document_entity(std::move(title), get_output_name(std::move(file_name)))
{
}

entity_kind template_document::do_get_kind() const noexcept
{
    return entity_kind::template_document;
}
