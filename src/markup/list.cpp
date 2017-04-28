// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/list.hpp>

#include "html_helper.hpp"

using namespace standardese::markup;

void list_item::do_append_html(std::string& result) const
{
    detail::append_newl(result);
    detail::append_html_open(result, "li", id(), "");

    detail::append_container(result, *this);

    result += "</li>\n";
}

void unordered_list::do_append_html(std::string& result) const
{
    detail::append_newl(result);
    detail::append_html_open(result, "ul", id(), "");

    detail::append_container(result, *this);

    result += "</ul>\n";
}

void ordered_list::do_append_html(std::string& result) const
{
    detail::append_newl(result);
    detail::append_html_open(result, "ol", id(), "");

    detail::append_container(result, *this);

    result += "</ol>\n";
}
