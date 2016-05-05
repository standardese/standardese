// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_STRING_HPP_INCLUDED
#define STANDARDESE_STRING_HPP_INCLUDED

#include <cstring>
#include <clang-c/CXString.h>

#include <standardese/noexcept.hpp>

namespace standardese
{
    /// Wrapper around CXString used for safe access.
    class string
    {
    public:
        string(CXString str)
        : cx_str_(str)
        {
            str_ = clang_getCString(cx_str_);
        }

        string(string &&) = delete;
        string& operator=(string &&) = delete;

        ~string() STANDARDESE_NOEXCEPT
        {
            clang_disposeString(cx_str_);
        }

        const char* get() const STANDARDESE_NOEXCEPT
        {
            return str_ ? str_ : "";
        }

        operator const char*() const STANDARDESE_NOEXCEPT
        {
            return get();
        }

    private:
        CXString cx_str_;
        const char *str_;
    };

    inline bool operator==(const string &a, const string &b) STANDARDESE_NOEXCEPT
    {
        return std::strcmp(a, b) == 0;
    }

    inline bool operator==(const string &a, const char *b) STANDARDESE_NOEXCEPT
    {
        return std::strcmp(a, b) == 0;
    }

    inline bool operator==(const char *a, const string &b) STANDARDESE_NOEXCEPT
    {
        return std::strcmp(a, b) == 0;
    }

    inline bool operator!=(const string &a, const string &b) STANDARDESE_NOEXCEPT
    {
        return !(a == b);
    }

    inline bool operator!=(const string &a, const char *b) STANDARDESE_NOEXCEPT
    {
        return !(a == b);
    }

    inline bool operator!=(const char *a, const string &b) STANDARDESE_NOEXCEPT
    {
        return !(a == b);
    }
} // namespace standardese

#endif // STANDARDESE_STRING_HPP_INCLUDED
