// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/parser.hpp>

using namespace standardese;

const char* const cpp_standard::cpp_98 = "-std=c++98";
const char* const cpp_standard::cpp_03 = "-std=c++03";
const char* const cpp_standard::cpp_11 = "-std=c++11";
const char* const cpp_standard::cpp_14 = "-std=c++14";

parser::parser()
: index_(clang_createIndex(1, 1))
{}

translation_unit parser::parse(const char *path, const char *standard) const
{
    const char* args[] = {"-x", "c++", standard};

    auto tu = clang_parseTranslationUnit(index_.get(), path, args, 3, nullptr, 0,
                                         CXTranslationUnit_Incomplete | CXTranslationUnit_DetailedPreprocessingRecord);

    return translation_unit(tu, path);
}

void parser::deleter::operator()(CXIndex idx) const STANDARDESE_NOEXCEPT
{
    clang_disposeIndex(idx);
}
