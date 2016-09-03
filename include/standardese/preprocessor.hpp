// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_PREPROCESSOR_HPP_INCLUDED
#define STANDARDESE_PREPROCESSOR_HPP_INCLUDED

#include <string>
#include <unordered_set>

#include <standardese/noexcept.hpp>

namespace standardese
{
    struct compile_config;

    class preprocessor
    {
    public:
        std::string preprocess(const compile_config& c, const char* full_path,
                               const std::string& source) const;

        void add_preprocess_directory(std::string dir)
        {
            preprocess_dirs_.insert(std::move(dir));
        }

        bool is_preprocess_directory(const std::string& dir) const STANDARDESE_NOEXCEPT
        {
            return preprocess_dirs_.count(dir) != 0;
        }

    private:
        std::unordered_set<std::string> preprocess_dirs_;
    };
} // namespace standardese

#endif // STANDARDESE_DETAIL_PREPROCESSOR_HPP_INCLUDED
