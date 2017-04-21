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
    result += "<article";
    detail::append_html_id(result, id(), "file-documentation");
    result += ">";

    detail::append_container(result, *this);

    result += "</article>\n";
}

void entity_documentation::do_append_html(std::string& result) const
{
    // <section> represents a semantic section in the website
    detail::append_newl(result);
    result += "<section";
    detail::append_html_id(result, id(), "entity-documentation");
    result += ">";

    detail::append_container(result, *this);

    result += "</section>\n";
}
