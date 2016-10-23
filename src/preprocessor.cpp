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
    std::string get_command(const compile_config& c, const char* full_path)
    {
        std::string cmd("clang++ -E -C ");
        for (auto flag : c.get_flags())
        {
            cmd += flag;
            cmd += ' ';
        }

        cmd += full_path;
        return cmd;
    }
}

std::string preprocessor::preprocess(const parser& p, const compile_config& c,
                                     const char* full_path) const
{
    std::string preprocessed;

    auto    cmd = get_command(c, full_path);
    Process process(cmd, "", [&](const char* str, std::size_t n) { preprocessed.append(str, n); },
                    [&](const char* str, std::size_t n) {
                        p.get_logger()->error("[preprocessor] {}", std::string(str, n));
                    });

    auto exit_code = process.get_exit_status();
    if (exit_code != 0)
        throw process_error(cmd, exit_code);

    return preprocessed;
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
