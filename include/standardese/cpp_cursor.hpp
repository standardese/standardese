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

        string get_usr() const STANDARDESE_NOEXCEPT
        {
            return clang_getCursorUSR(*this);
        }
    };

    inline bool operator==(const cpp_cursor& a, const cpp_cursor& b) STANDARDESE_NOEXCEPT
    {
        assert(!a.get_usr().empty() && !b.get_usr().empty());
        return a.get_usr() == b.get_usr();
    }

    inline bool operator!=(const cpp_cursor& a, const cpp_cursor& b) STANDARDESE_NOEXCEPT
    {
        return !(a == b);
    }

    inline bool operator<(const cpp_cursor& a, const cpp_cursor& b) STANDARDESE_NOEXCEPT
    {
        assert(!a.get_usr().empty() && !b.get_usr().empty());
        return a.get_usr() < b.get_usr();
    }

    inline bool operator<=(const cpp_cursor& a, const cpp_cursor& b) STANDARDESE_NOEXCEPT
    {
        return !(b < a);
    }

    inline bool operator>(const cpp_cursor& a, const cpp_cursor& b) STANDARDESE_NOEXCEPT
    {
        return b < a;
    }

    inline bool operator>=(const cpp_cursor& a, const cpp_cursor& b) STANDARDESE_NOEXCEPT
    {
        return !(b < a);
    }
} // namespace standardese

#endif //STANDARDESE_CPP_CURSOR_HPP_INCLUDED
