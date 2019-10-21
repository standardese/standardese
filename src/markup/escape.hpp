// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_ESCAPE_HPP_INCLUDED
#define STANDARDESE_MARKUP_ESCAPE_HPP_INCLUDED

#include <cstdio>
#include <cstring>
#include <ostream>

namespace standardese
{
namespace markup
{
    namespace detail
    {
        inline void write_html_text(std::ostream& out, const char* str)
        {
            // implements rule 1 here:
            // https://www.owasp.org/index.php/XSS_(Cross_Site_Scripting)_Prevention_Cheat_Sheet
            for (auto ptr = str; *ptr; ++ptr)
            {
                auto c = *ptr;
                if (c == '&')
                    out << "&amp;";
                else if (c == '<')
                    out << "&lt;";
                else if (c == '>')
                    out << "&gt;";
                else if (c == '"')
                    out << "&quot;";
                else if (c == '\'')
                    out << "&#x27;";
                else if (c == '/')
                    out << "&#x2F;";
                else
                    out << c;
            }
        }

        inline bool needs_url_escaping(char c)
        {
            // don't escape reserved URL characters
            // don't escape safe URL characters
            char safe[] = "-_.+!*(),%#@?=;:/,+$"
                          "0123456789"
                          "abcdefghijklmnopqrstuvwxyz"
                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            return std::strchr(safe, c) == nullptr;
        }

        inline void write_html_url(std::ostream& out, const char* url)
        {
            for (auto ptr = url; *ptr; ++ptr)
            {
                auto c = *ptr;
                if (c == '&')
                    out << "&amp;";
                else if (c == '\'')
                    out << "&#x27";
                else if (needs_url_escaping(c))
                {
                    char buf[3];
                    std::snprintf(buf, 3, "%02X", unsigned(c));
                    out << "%";
                    out << buf;
                }
                else
                    out << c;
            }
        }
    } // namespace detail
} // namespace markup
} // namespace standardese

#endif // STANDARDESE_MARKUP_ESCAPE_HPP_INCLUDED
