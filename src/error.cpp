// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/error.hpp>

#include <standardese/string.hpp>

using namespace standardese;

source_location::source_location(CXSourceLocation location, std::string entity)
: entity_name(std::move(entity))
{
    CXFile file;
    clang_getSpellingLocation(location, &file, &line, nullptr, nullptr);
    file_name = string(clang_getFileName(file)).get();
}
