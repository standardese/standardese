// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/heading.hpp>

#include "html_helper.hpp"

using namespace standardese::markup;

void heading::do_append_html(std::string& result) const
{
    detail::append_heading(result, *this, "h4", "");
}

void subheading::do_append_html(std::string& result) const
{
    detail::append_heading(result, *this, "h5", "");
}
