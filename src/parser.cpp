// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/parser.hpp>

#include <spdlog/sinks/null_sink.h>

#include <standardese/error.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

translation_unit parser::parse(const char* path, const compile_config& c) const
{
    auto args = c.get_flags();

    CXTranslationUnit tu;
    auto              error =
        clang_parseTranslationUnit2(index_.get(), path, args.data(), static_cast<int>(args.size()),
                                    nullptr, 0, CXTranslationUnit_Incomplete
                                                    | CXTranslationUnit_DetailedPreprocessingRecord,
                                    &tu);
    if (error != CXError_Success)
        throw libclang_error(error, "CXTranslationUnit (" + std::string(path) + ")");

    cpp_ptr<cpp_file> file(new cpp_file(clang_getTranslationUnitCursor(tu), path));
    auto              file_ptr = file.get();
    files_.add_file(std::move(file));

    return translation_unit(*this, tu, path, file_ptr);
}

parser::parser()
: parser(std::make_shared<spdlog::logger>("null", std::make_shared<spdlog::sinks::null_sink_mt>()))
{
}

parser::parser(std::shared_ptr<spdlog::logger> logger)
: index_(clang_createIndex(1, 1)), logger_(std::move(logger))
{
}

parser::~parser() STANDARDESE_NOEXCEPT
{
}

void parser::deleter::operator()(CXIndex idx) const STANDARDESE_NOEXCEPT
{
    clang_disposeIndex(idx);
}
