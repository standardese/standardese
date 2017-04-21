// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_HTML_HELPER_HPP_INCLUDED
#define STANDARDESE_MARKUP_HTML_HELPER_HPP_INCLUDED

#include <string>

namespace standardese
{
    namespace markup
    {
        class block_id;

        namespace detail
        {
            // escapes the string for use in an HTML element content or in a quoted "normal" attribute
            // implements rule 1 here: https://www.owasp.org/index.php/XSS_(Cross_Site_Scripting)_Prevention_Cheat_Sheet
            std::string escape_html(const std::string& str);

            // escapes the string for use in an URL
            // does not escape reserved characters of an URL
            std::string escape_url(const std::string& str);

            // writes id attributes, prepends space
            void append_html_id(std::string& result, const block_id& id, std::string class_name);

            void append_newl(std::string& result);

            template <typename T>
            void append_container(std::string& result, const T& container)
            {
                for (auto& child : container)
                    child.append_html(result);
            }
        } // namespace detail
    }
} // namespace standardese::markup

#endif // STANDARDESE_MARKUP_HTML_HELPER_HPP_INCLUDED
