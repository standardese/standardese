// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_TEST_PARSER_HPP_INCLUDED
#define STANDARDESE_TEST_PARSER_HPP_INCLUDED

#include <fstream>

#include <standardese/cpp_entity.hpp>
#include <standardese/parser.hpp>
#include <standardese/translation_unit.hpp>

inline standardese::translation_unit parse(standardese::parser &p, const char *name, const char *code,
                                           const standardese::compile_config &c = standardese::cpp_standard::cpp_14)
{
    std::ofstream file(name);
    file << code;
    file.close();

    return p.parse(name, c);
}

template <typename T>
std::vector<standardese::cpp_ptr<T>> parse_entity(standardese::translation_unit &unit, CXCursorKind kind)
{
    std::vector<standardese::cpp_ptr<T>> result;

    unit.visit([&](CXCursor cur, CXCursor)
               {
                    if (clang_getCursorKind(cur) == kind)
                    {
                        result.push_back(T::parse(cur));
                        return CXChildVisit_Continue;
                    }
                    return CXChildVisit_Recurse;
               });

    return result;
}

#endif // STANDARDESE_TEST_PARSER_HPP_INCLUDED
