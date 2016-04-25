// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/translation_unit.hpp>

using namespace standardese;

translation_unit::translation_unit(CXTranslationUnit tu, const char *path) STANDARDESE_NOEXCEPT
: tu_(tu)
{
    file_ = clang_getFile(tu_.get(), path);
    detail::validate(file_);
}

void translation_unit::deleter::operator()(CXTranslationUnit tu) const STANDARDESE_NOEXCEPT
{
    clang_disposeTranslationUnit(tu);
}
