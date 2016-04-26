// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_PARSER_HPP_INCLUDED
#define STANDARDESE_PARSER_HPP_INCLUDED

#include <clang-c/Index.h>
#include <memory>
#include <utility>

#include <standardese/detail/wrapper.hpp>
#include "cpp_namespace.hpp"

namespace standardese
{
    class translation_unit;
    class cpp_file;
    class cpp_namespace;

    /// C++ standard to be used
    struct cpp_standard
    {
        static const char* const cpp_98;
        static const char* const cpp_03;
        static const char* const cpp_11;
        static const char* const cpp_14;
    };

    /// Parser class used for parsing the C++ classes.
    /// The parser object must live as long as all the translation units.
    class parser
    {
    public:
        parser();

        parser(parser&&) = delete;
        parser(const parser&) = delete;

        ~parser() STANDARDESE_NOEXCEPT;

        parser& operator=(parser&&) = delete;
        parser& operator=(const parser&) = delete;

        /// Parses a translation unit.
        /// standard must be one of the cpp_standard values.
        translation_unit parse(const char *path, const char *standard) const;

        void register_file(cpp_file &f) const;

        // void(const cpp_file &file)
        template <typename Fnc>
        void for_each_file(Fnc f)
        {
            auto cb = [](const cpp_file &f, void *data)
            {
                (*static_cast<Fnc*>(data))(f);
            };

            for_each_file(cb, &f);
        }

        void register_namespace(cpp_namespace &n) const;

        // void(cpp_name namespace_name)
        template <typename Fnc>
        void for_each_namespace(Fnc f)
        {
            auto cb = [](const cpp_name &n, void *data)
            {
                (*static_cast<Fnc*>(data))(n);
            };

            for_each_namespace(cb, &f);
        }

        // void(const cpp_entity &e)
        // returns one namespace object of that name
        template <typename Fnc>
        const cpp_namespace* for_each_in_namespace(const cpp_name &n, Fnc f)
        {
            auto cb = [](const cpp_entity &e, void *data)
            {
                (*static_cast<Fnc*>(data))(e);
            };

            return for_each_in_namespace(n, cb, &f);
        }

    private:
        using file_callback = void(*)(const cpp_file&, void*);
        void for_each_file(file_callback cb, void* data);

        using namespace_callback = void(*)(const cpp_name&, void*);
        void for_each_namespace(namespace_callback cb, void *data);

        using in_namespace_callback = void(*)(const cpp_entity&, void*);
        const cpp_namespace* for_each_in_namespace(const cpp_name &n, in_namespace_callback cb, void *data);

        struct deleter
        {
            void operator()(CXIndex idx) const STANDARDESE_NOEXCEPT;
        };

        struct impl;

        detail::wrapper<CXIndex, deleter> index_;
        std::unique_ptr<impl> pimpl_;
    };
} // namespace standardese

#endif // STANDARDESE_PARSER_HPP_INCLUDED
