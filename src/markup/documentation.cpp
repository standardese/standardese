// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/documentation.hpp>

#include "html_helper.hpp"

using namespace standardese::markup;

void file_documentation::do_append_html(std::string& result) const
{
    // <article> represents the actual content of a website
    detail::append_newl(result);
    detail::append_html_open(result, "article", id(), "file-documentation");

    if (heading())
        detail::append_heading(result, heading().value(), "h1", "file-documentation-heading");

    detail::append_container(result, *this);

    result += "</article>\n";
}

namespace
{
    unsigned no_parents(const entity_documentation& entity)
    {
        auto result = 0u;
        for (auto cur = entity.parent(); cur; cur = cur.value().parent())
            ++result;
        return result;
    }
}

void entity_documentation::do_append_html(std::string& result) const
{
    // <section> represents a semantic section in the website
    detail::append_newl(result);
    detail::append_html_open(result, "section", id(), "entity-documentation");

    if (heading())
    {
        // allow 2 parent entities for level 2: file + document
        auto tag = no_parents(*this) < 3u ? "h2" : "h3";
        detail::append_heading(result, heading().value(), tag, "entity-documentation-heading");
    }

    detail::append_container(result, *this);

    result += "</section>\n";
}
