// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "html_helper.hpp"

using namespace standardese::markup;

std::string detail::escape_html_element_content(const std::string& str)
{
    std::string result;
    for (auto c : str)
    {
        if (c == '&')
            result += "&amp;";
        else if (c == '<')
            result += "&lt;";
        else if (c == '>')
            result += "&gt;";
        else if (c == '"')
            result += "&quot;";
        else if (c == '\'')
            result += "&#x27;";
        else if (c == '/')
            result += "&#x2F;";
        else
            result += c;
    }
    return result;
}
