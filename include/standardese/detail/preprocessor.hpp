// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_PREPROCESSOR_HPP_INCLUDED
#define STANDARDESE_DETAIL_PREPROCESSOR_HPP_INCLUDED

#include <string>

namespace standardese
{
    struct compile_config;

    namespace detail
    {
        std::string preprocess(const compile_config& c, const char* full_path,
                               const std::string& source);
    } // namespace detail
} // namespace standardese

#endif // STANDARDESE_DETAIL_PREPROCESSOR_HPP_INCLUDED
