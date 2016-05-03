// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_CURSOR_HPP_INCLUDED
#define STANDARDESE_CPP_CURSOR_HPP_INCLUDED

#include <clang-c/Index.h>

#include <standardese/noexcept.hpp>

namespace standardese
{
    /// A C++ struct that acts as CXCursor.
    /// It can be forward declared however.
    struct cpp_cursor : CXCursor
    {
        cpp_cursor(CXCursor cur) STANDARDESE_NOEXCEPT
        : CXCursor(cur) {}
    };
} // namespace standardese

#endif //STANDARDESE_CPP_CURSOR_HPP_INCLUDED
