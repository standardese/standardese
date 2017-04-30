// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/quote.hpp>

#include "html_helper.hpp"

using namespace standardese::markup;

void block_quote::do_append_html(std::string& result) const
{
    detail::append_newl(result);
    detail::append_html_open(result, "blockquote", id(), "");

    detail::append_container(result, *this);

    result += "</blockquote>\n";
}
