// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_PARSE_UTILS_HPP_INCLUDED
#define STANDARDESE_PARSE_UTILS_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>

namespace standardese
{
    struct cpp_cursor;

    namespace detail
    {
        // obtains the name from cursor
        cpp_name parse_name(cpp_cursor cur);

        // obtains the comment from cursor
        cpp_comment parse_comment(cpp_cursor cur);
    } // namespace detail
} // namespace standardese

#endif // STANDARDESE_PARSE_UTILS_HPP_INCLUDED
