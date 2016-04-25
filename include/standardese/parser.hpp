// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_PARSER_HPP_INCLUDED
#define STANDARDESE_PARSER_HPP_INCLUDED

#include <clang-c/Index.h>
#include <utility>

#include <standardese/detail/wrapper.hpp>
#include <standardese/translation_unit.hpp>

namespace standardese
{
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

        /// Parses a translation unit.
        /// standard must be one of the cpp_standard values.
        translation_unit parse(const char *path, const char *standard) const;

    private:
        struct deleter
        {
            void operator()(CXIndex idx) const STANDARDESE_NOEXCEPT;
        };

        detail::wrapper<CXIndex, deleter> index_;
    };
} // namespace standardese

#endif // STANDARDESE_PARSER_HPP_INCLUDED
