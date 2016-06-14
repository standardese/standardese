// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_TEST_PARSER_HPP_INCLUDED
#define STANDARDESE_TEST_PARSER_HPP_INCLUDED

#include <fstream>

#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/parser.hpp>
#include <standardese/string.hpp>
#include <standardese/translation_unit.hpp>

inline standardese::translation_unit parse(
    standardese::parser& p, const char* name, const char* code,
    const standardese::compile_config& c = standardese::cpp_standard::cpp_14)
{
    std::ofstream file(name);
    file << code;
    file.close();

    return p.parse(name, c);
}

template <typename T>
std::vector<standardese::cpp_ptr<T>> parse_entity(standardese::translation_unit& unit,
                                                  CXCursorKind                   kind)
{
    std::vector<standardese::cpp_ptr<T>> result;

    standardese::detail::visit_tu(unit.get_cxunit(), unit.get_cxfile(),
                                  [&](CXCursor cur, CXCursor) {
                                      if (clang_getCursorKind(cur) == kind)
                                      {
                                          result.push_back(T::parse(unit, cur, unit.get_file()));
                                          return CXChildVisit_Continue;
                                      }
                                      return CXChildVisit_Recurse;
                                  });

    return result;
}

template <typename Func>
void for_each(const standardese::cpp_entity& e, Func f)
{
    using namespace standardese;

    if (e.get_entity_type() == cpp_entity::file_t)
        for (auto& child : static_cast<const cpp_file&>(e))
            for_each(child, f);
    else if (e.get_entity_type() == cpp_entity::namespace_t)
        for (auto& child : static_cast<const cpp_namespace&>(e))
            for_each(child, f);
    else
        f(e);
}

#endif // STANDARDESE_TEST_PARSER_HPP_INCLUDED
