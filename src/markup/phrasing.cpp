// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/phrasing.hpp>

#include "html_helper.hpp"

using namespace standardese::markup;

void text::do_append_html(std::string& result) const
{
    result += detail::escape_html_element_content(string());
}

void emphasis::do_append_html(std::string& result) const
{
    result += "<em>";
    detail::append_container(result, *this);
    result += "</em>";
}

void strong_emphasis::do_append_html(std::string& result) const
{
    result += "<strong>";
    detail::append_container(result, *this);
    result += "</strong>";
}

void definition::do_append_html(std::string& result) const
{
    result += "<dfn>";
    detail::append_container(result, *this);
    result += "</dfn>";
}

void code::do_append_html(std::string& result) const
{
    result += "<code>";
    detail::append_container(result, *this);
    result += "</code>";
}
