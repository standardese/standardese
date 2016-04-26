// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/translation_unit.hpp>

using namespace standardese;

translation_unit::translation_unit(CXTranslationUnit tu, const char *path)
: tu_(tu), path_(path)
{}

CXFile translation_unit::get_cxfile() const STANDARDESE_NOEXCEPT
{
    auto file = clang_getFile(tu_.get(), get_path());
    detail::validate(file);
    return file;
}

cpp_ptr<cpp_file> translation_unit::get_cpp_file() const
{
    return detail::make_ptr<cpp_file>(get_path());
}

void translation_unit::deleter::operator()(CXTranslationUnit tu) const STANDARDESE_NOEXCEPT
{
    clang_disposeTranslationUnit(tu);
}
