// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_FILESYSTEM_HPP_INCLUDED
#define STANDARDESE_FILESYSTEM_HPP_INCLUDED

#include <string>
#include <vector>

#include <boost/filesystem.hpp>

namespace standardese_tool
{
    namespace fs = boost::filesystem;

    // blacklist of certain extensions, files, ...
    using blacklist = std::vector<std::string>;

    namespace detail
    {
        inline bool is_valid(const fs::path& path, const fs::path& root,
                             const blacklist& extensions, const blacklist& files,
                             const blacklist& dirs)
        {
#if BOOST_VERSION / 100 % 1000 < 60
            auto relative = path.c_str() + root.native().size();
            if (*relative == '/' || *relative == '\\')
                ++relative;
#else
            auto relative = fs::relative(path, root);
#endif

            if (fs::is_directory(path))
            {
                for (auto& d : dirs)
                    if (relative == d)
                        return false;
            }
            else
            {
                for (auto& f : files)
                    if (relative == f)
                        return false;

                auto ext = path.extension();
                for (auto& e : extensions)
                    if (ext == e || (ext.empty() && e == "."))
                        return false;
            }

            return true;
        }
    } // namespace detail

    // a path is determined valid through the blacklists
    // if given path is normal file and valid, calls f for it
    // otherwise recursively traverses through the given directory and calls f for each valid file
    // returns false if path was a normal file that was marked as invalid, true otherwise
    template <typename Fun>
    bool handle_path(const fs::path& path, const blacklist& extensions, const blacklist& files,
                     blacklist dirs, Fun f)
    {
        // remove trailing slash if any
        // otherwise Boost.Filesystem can't handle it
        for (auto& dir : dirs)
        {
            if (dir.back() == '/' || dir.back() == '\\')
                dir.pop_back();
        }

        if (fs::is_directory(path))
        {
            auto end = fs::recursive_directory_iterator();
            for (auto iter = fs::recursive_directory_iterator(path); iter != end; ++iter)
            {
                auto& cur = iter->path();

                if (fs::is_directory(cur))
                {
                    if (!detail::is_valid(cur, path, extensions, files, dirs))
                        iter.no_push();
                }
                else if (detail::is_valid(cur, path, extensions, files, dirs))
                {
                    f(cur);
                }
            }
        }
        else if (!fs::exists(path))
            throw std::runtime_error("file '" + path.generic_string() + "' does not exist");
        else if (detail::is_valid(path, "", extensions, files, dirs))
        {
            f(path);
        }
        else
            return false;

        return true;
    }
} // namespace standardese_tool

#endif // STANDARDESE_FILESYSTEM_HPP_INCLUDED
