// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_STRING_HPP_INCLUDED
#define STANDARDESE_STRING_HPP_INCLUDED

#include <cstring>
#include <new>
#include <string>
#include <type_traits>

#include <clang-c/CXString.h>

#include <standardese/noexcept.hpp>

namespace standardese
{
    /// Wrapper around CXString used for safe access.
    class string
    {
    public:
        string(const char *str)
        : length_(std::strlen(str)), type_(std_string)
        {
            ::new(get_storage()) std::string(str, length_);
        }

        template <std::size_t N>
        string(const char(&str)[N])
        : length_(N), type_(literal)
        {
            ::new(get_storage()) const char*(str);
        }

        string(std::string str)
        : length_(str.length()), type_(std_string)
        {
            ::new(get_storage()) std::string(std::move(str));
        }

        string(CXString str) STANDARDESE_NOEXCEPT
        : length_(std::strlen(clang_getCString(str))),
          type_(cx_string)
        {
            ::new(get_storage()) CXString(str);
        }

        string(const string &other)
        : length_(other.length_), type_(std_string)
        {
            ::new(get_storage()) std::string(other.c_str());
        }

        ~string() STANDARDESE_NOEXCEPT
        {
            free();
        }

        string& operator=(const string &other)
        {
            std::string str(other.c_str());
            if (type_ == std_string)
                *static_cast<std::string*>(get_storage()) = std::move(str);
            else
            {
                free();
                ::new(get_storage()) std::string(std::move(str));
            }
            length_ = other.length_;

            return *this;
        }

        const char* c_str() const STANDARDESE_NOEXCEPT
        {
            if (type_ == std_string)
                return static_cast<const std::string*>(get_storage())->c_str();
            else if (type_ == literal)
                return *static_cast<const char* const *>(get_storage());
            return clang_getCString(*static_cast<const CXString*>(get_storage()));
        }

        bool empty() const STANDARDESE_NOEXCEPT
        {
            return  length_ == 0u;
        }

        std::size_t length() const STANDARDESE_NOEXCEPT
        {
            return length_;
        }

        const char* begin() const STANDARDESE_NOEXCEPT
        {
            return c_str();
        }

        const char* end() const STANDARDESE_NOEXCEPT
        {
            return begin() + length_;
        }

    private:
        void* get_storage() STANDARDESE_NOEXCEPT
        {
            return static_cast<void*>(&storage_);
        }

        const void* get_storage() const STANDARDESE_NOEXCEPT
        {
            return static_cast<const void*>(&storage_);
        }

        void free() STANDARDESE_NOEXCEPT
        {
            if (type_ == cx_string)
                clang_disposeString(*static_cast<CXString*>(get_storage()));
            else if (type_ == std_string)
                static_cast<std::string*>(get_storage())->std::string::~string();
        }

        std::aligned_storage<(sizeof(std::string) > sizeof(CXString))
                            ? sizeof(std::string) : sizeof(CXString)>::type storage_;
        std::size_t length_;
        enum
        {
            cx_string,
            std_string,
            literal,
        } type_;
    };

    inline bool operator==(const string &a, const string &b) STANDARDESE_NOEXCEPT
    {
        return std::strcmp(a.c_str(), b.c_str()) == 0;
    }

    inline bool operator==(const string &a, const char *b) STANDARDESE_NOEXCEPT
    {
        return std::strcmp(a.c_str(), b) == 0;
    }

    inline bool operator==(const char *a, const string &b) STANDARDESE_NOEXCEPT
    {
        return std::strcmp(a, b.c_str()) == 0;
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

    inline bool operator<(const string &a, const string &b) STANDARDESE_NOEXCEPT
    {
        return std::strcmp(a.c_str(), b.c_str()) < 0;
    }

    inline bool operator>(const string &a, const string &b) STANDARDESE_NOEXCEPT
    {
        return std::strcmp(a.c_str(), b.c_str()) > 0;
    }

    inline bool operator>=(const string &a, const string &b) STANDARDESE_NOEXCEPT
    {
        return std::strcmp(a.c_str(), b.c_str()) >= 0;
    }

    inline bool operator<=(const string &a, const string &b) STANDARDESE_NOEXCEPT
    {
        return std::strcmp(a.c_str(), b.c_str()) <= 0;
    }
} // namespace standardese

namespace Catch
{
    inline std::string toString(const standardese::string &str)
    {
        return std::string("\"") + str.c_str() + '"';
    }
} // namespace Catch

#endif // STANDARDESE_STRING_HPP_INCLUDED
