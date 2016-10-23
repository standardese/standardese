// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/preprocessor.hpp>

#include <boost/config.hpp>
#include <boost/filesystem.hpp>
#include <boost/version.hpp>

#if (BOOST_VERSION / 100000) != 1
#error "require Boost 1.x"
#endif

#if ((BOOST_VERSION / 100) % 1000) < 55
#warning "Boost less than 1.55 isn't tested"
#endif

#include <standardese/detail/tokenizer.hpp>
#include <standardese/config.hpp>
#include <standardese/error.hpp>
#include <standardese/parser.hpp>

// treat the tiny-process-library as header only
#include <process.hpp>
#include <process.cpp>
#ifdef BOOST_WINDOWS
#include <process_win.cpp>
#else
#include <process_unix.cpp>
#endif

using namespace standardese;

namespace fs = boost::filesystem;

namespace
{
    std::string read_source(const char* full_path)
    {
        std::filebuf filebuf;
        filebuf.open(full_path, std::ios_base::in);
        assert(filebuf.is_open());

        std::string source;
        for (std::istreambuf_iterator<char> iter(&filebuf), end = {}; iter != end; ++iter)
        {
            // handle backslashes
            if (*iter == '\\')
            {
                ++iter;
                if (*iter == '\n')
                    ++iter; // newline is escaped, ignore
                else
                    source += '\\'; // "normal" backslash
            }
            source += *iter;
        }
        if (source.back() != '\n')
            source.push_back('\n');
        return source;
    }
}

std::string preprocessor::preprocess(const parser&, const compile_config& c,
                                     const char* full_path) const
{
    return read_source(full_path);
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
