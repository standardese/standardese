// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_PARSER_HPP_INCLUDED
#define STANDARDESE_PARSER_HPP_INCLUDED

#include <clang-c/Index.h>
#include <memory>
#include <utility>

#include <standardese/detail/wrapper.hpp>

namespace standardese
{
    class translation_unit;
    class cpp_file;

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

        using file_callback = void(*)(cpp_file&, void*);

        void for_each_file(file_callback cb, void* data);

        template <typename Fnc>
        void for_each_file(Fnc f)
        {
            auto cb = [](cpp_file &f, void *data)
            {
                (*static_cast<Fnc*>(data))(f);
            };

            for_each_file(cb, &f);
        }

    private:
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
