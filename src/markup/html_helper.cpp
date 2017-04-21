// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "html_helper.hpp"

#include <cstring>
#include <cstdio>

using namespace standardese::markup;

std::string detail::escape_html(const std::string& str)
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

namespace
{
    bool needs_url_escaping(char c)
    {
        // don't escape reserved URL characters
        // don't escape safe URL characters
        char safe[] = "-_.+!*(),%#@?=;:/,+$"
                      "0123456789"
                      "abcdefghijklmnopqrstuvwxyz"
                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        return std::strchr(safe, c) == nullptr;
    }
}

std::string detail::escape_url(const std::string& str)
{
    std::string result;
    for (auto c : str)
    {
        if (c == '&')
            result += "&amp;";
        else if (c == '\'')
            result += "&#x27";
        else if (needs_url_escaping(c))
        {
            char buf[3];
            std::snprintf(buf, 3, "%02X", unsigned(c));
            result += "%";
            result += buf;
        }
        else
            result += c;
    }
    return result;
}
