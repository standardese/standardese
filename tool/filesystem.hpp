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
    using whitelist = blacklist;

    namespace detail
    {
        inline fs::path get_relative_path(const fs::path& path, const fs::path& root)
        {
#if BOOST_VERSION / 100 % 1000 < 60
            auto relative = path.c_str() + root.native().size();
            if (*relative == '/' || *relative == '\\')
                ++relative;
            return relative;
#else
            return fs::relative(path, root);
#endif
        }

        inline bool is_valid(const fs::path& path, const fs::path& relative,
                             const blacklist& extensions, const blacklist& files,
                             const blacklist& dirs, bool blacklist_dotfiles)
        {
            if (blacklist_dotfiles && path.filename().generic_string()[0] == '.')
                return false;

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

        inline bool is_source_file(const fs::path& path, const whitelist& source_extensions)
        {
            auto ext = path.extension();
            for (auto& e : source_extensions)
                if (ext == e)
                    return true;
            return false;
        }
    } // namespace detail

    // a path is determined valid through the blacklists
    // if given path is normal file and valid, calls f for it
    // otherwise recursively traverses through the given directory and calls f for each valid file
    // returns false if path was a normal file that was marked as invalid, true otherwise
    template <typename Fun>
    bool handle_path(const fs::path& path, const whitelist& source_extensions,
                     const blacklist& extensions, const blacklist& files, blacklist dirs,
                     bool blacklist_dotfiles, bool force_blacklist, Fun f)
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
                auto& cur      = iter->path();
                auto  relative = detail::get_relative_path(cur, path);

                if (fs::is_directory(cur))
                {
                    if (!detail::is_valid(cur, relative, extensions, files, dirs,
                                          blacklist_dotfiles))
                        iter.no_push();
                }
                else if (detail::is_valid(cur, relative, extensions, files, dirs,
                                          blacklist_dotfiles))
                {
                    f(detail::is_source_file(cur, source_extensions), cur, relative);
                }
            }
        }
        else if (!fs::exists(path))
            throw std::runtime_error("file '" + path.generic_string() + "' does not exist");
        else if (!force_blacklist
                 || detail::is_valid(path, "", extensions, files, dirs, blacklist_dotfiles))
        {
            // return only the filename of the path as relative path
            f(detail::is_source_file(path, source_extensions), path, path.filename());
        }
        else
            return false;

        return true;
    }
} // namespace standardese_tool

#endif // STANDARDESE_FILESYSTEM_HPP_INCLUDED
