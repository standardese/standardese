// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_CURSOR_HPP_INCLUDED
#define STANDARDESE_CPP_CURSOR_HPP_INCLUDED

#include <cassert>

#include <clang-c/Index.h>

#include <standardese/noexcept.hpp>
#include <standardese/string.hpp>

namespace standardese
{
    /// A C++ struct that acts as CXCursor.
    /// It can be forward declared however.
    struct cpp_cursor : CXCursor
    {
        cpp_cursor() STANDARDESE_NOEXCEPT : cpp_cursor(clang_getNullCursor())
        {
        }

        cpp_cursor(CXCursor cur) STANDARDESE_NOEXCEPT : CXCursor(cur)
        {
        }
    };

    inline bool operator==(const cpp_cursor& a, const cpp_cursor& b) STANDARDESE_NOEXCEPT
    {
        return clang_equalCursors(a, b) != 0u;
    }

    inline bool operator!=(const cpp_cursor& a, const cpp_cursor& b) STANDARDESE_NOEXCEPT
    {
        return !(a == b);
    }
} // namespace standardese

#endif //STANDARDESE_CPP_CURSOR_HPP_INCLUDED
