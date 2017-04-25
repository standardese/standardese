// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/heading.hpp>

#include "html_helper.hpp"

using namespace standardese::markup;

void heading::do_append_html(std::string& result) const
{
    detail::append_newl(result);
    detail::append_html_open(result, "h4", id(), "heading");

    detail::append_container(result, *this);

    result += "</h4>\n";
}

void subheading::do_append_html(std::string& result) const
{
    detail::append_newl(result);
    detail::append_html_open(result, "h5", id(), "subheading");

    detail::append_container(result, *this);

    result += "</h5>\n";
}
