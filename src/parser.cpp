// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/parser.hpp>

#include <standardese/detail/tokenizer.hpp>
#include <standardese/preprocessor.hpp>
#include <standardese/error.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

namespace
{
    CXTranslationUnit get_cxunit(CXIndex index, const compile_config& c, const char* full_path,
                                 const std::string& source)
    {
        auto args = c.get_flags();

        CXUnsavedFile file;
        file.Filename = full_path;
        file.Contents = source.c_str();
        file.Length   = source.length();

        CXTranslationUnit tu;
        auto              error = clang_parseTranslationUnit2(index, full_path, args.data(),
                                                 static_cast<int>(args.size()), &file, 1,
                                                 CXTranslationUnit_Incomplete, &tu);
        if (error != CXError_Success)
            throw libclang_error(error, "CXTranslationUnit (" + std::string(full_path) + ")");
        return tu;
    }
}

translation_unit parser::parse(const char* full_path, const compile_config& c,
                               const char* file_name) const
{
    file_name = file_name ? file_name : full_path;

    cpp_ptr<cpp_file> file(new cpp_file(file_name));
    auto              file_ptr = file.get();
    files_.add_file(std::move(file));

    auto preprocessed = preprocessor_.preprocess(*this, c, full_path, *file_ptr);
    auto tu           = get_cxunit(index_.get(), c, full_path, preprocessed);
    parse_comments(*this, file_name, preprocessed);

    file_ptr->wrapper_ = detail::tu_wrapper(tu);
    file_ptr->set_cursor(clang_getTranslationUnitCursor(tu));

    return translation_unit(*this, full_path, file_ptr);
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
