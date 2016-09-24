// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/preprocessor.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'sprintf' : format string '%ld' requires an argument of type 'long', but variadic argument 1 has type 'size_t'
#pragma warning(disable : 4477)
#endif

#include <boost/filesystem.hpp>
#include <boost/version.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if (BOOST_VERSION / 100000) != 1
#error "require Boost 1.x"
#endif

#if ((BOOST_VERSION / 100) % 1000) < 55
#warning "Boost less than 1.55 isn't tested"
#endif

#include <standardese/detail/raw_comment.hpp>
#include <standardese/config.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

namespace fs = boost::filesystem;

namespace
{
}

std::string preprocessor::preprocess(const compile_config& c, const char* full_path,
                                     const std::string& source, cpp_file& file) const
{
    return source;
}

void preprocessor::add_preprocess_directory(std::string dir)
{
    auto path = fs::system_complete(dir).normalize().generic_string();
    if (!path.empty() && path.back() == '.')
        path.pop_back();
    preprocess_dirs_.insert(std::move(path));
}

bool preprocessor::is_preprocess_directory(const std::string& dir) const STANDARDESE_NOEXCEPT
{
    return preprocess_dirs_.count(dir) != 0;
}
