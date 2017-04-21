// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/link.hpp>

#include "html_helper.hpp"

using namespace standardese::markup;

namespace
{
    void append_link_html(std::string& result, const link_base& link, const std::string& dest)
    {
        result += "<a";
        result += " href=\"" + detail::escape_url(dest) + '"';
        if (!link.title().empty())
            result += " title=\"" + detail::escape_html(link.title()) + '"';
        result += ">";
        detail::append_container(result, link);
        result += "</a>";
    }
}

void external_link::do_append_html(std::string& result) const
{
    append_link_html(result, *this, url());
}
