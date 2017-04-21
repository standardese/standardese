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
        /// \exclude
        namespace detail
        {
            // escapes the string for use in an html element content
            // implements rule 1 here: https://www.owasp.org/index.php/XSS_%28Cross_Site_Scripting%29_Prevention_Cheat_Sheet#RULE_.231_-_HTML_Escape_Before_Inserting_Untrusted_Data_into_HTML_Element_Content
            std::string escape_html_element_content(const std::string& str);

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
