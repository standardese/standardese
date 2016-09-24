// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/parser.hpp>

#include <fstream>
#include <spdlog/sinks/null_sink.h>

#include <standardese/detail/tokenizer.hpp>
#include <standardese/preprocessor.hpp>
#include <standardese/error.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

namespace
{
    std::string read_source(const char* full_path)
    {
        // need to open in binary mode as libclang does it (apparently)
        // otherwise the offsets are incompatible under Windows
        std::filebuf filebuf;
        filebuf.open(full_path, std::ios_base::in | std::ios_base::binary);
        assert(filebuf.is_open());

        auto source =
            std::string(std::istreambuf_iterator<char>(&filebuf), std::istreambuf_iterator<char>());
        if (source.back() != '\n')
            source.push_back('\n');
        return source;
    }

    CXTranslationUnit get_cxunit_preprcoessing(CXIndex index, const compile_config& c,
                                               const char* full_path)
    {
        auto args = c.get_flags();

        CXTranslationUnit tu;
        auto              error =
            clang_parseTranslationUnit2(index, full_path, args.data(),
                                        static_cast<int>(args.size()), nullptr, 0,
                                        CXTranslationUnit_Incomplete
                                            | CXTranslationUnit_DetailedPreprocessingRecord,
                                        &tu);
        if (error != CXError_Success)
            throw libclang_error(error, "CXTranslationUnit (" + std::string(full_path) + ")");
        return tu;
    }

    std::string get_macro(CXCursor cur)
    {
        unsigned begin, end;
        auto     file = detail::get_range(clang_getCursorExtent(cur), begin, end);
        if (!file)
            return "";

        std::filebuf buf;
        buf.open(string(clang_getFileName(file)).c_str(),
                 std::ios_base::in | std::ios_base::binary);
        assert(buf.is_open());
        buf.pubseekpos(begin);

        std::string res;
        for (; buf.sgetc() != '(' && !std::isspace(buf.sgetc()); buf.sbumpc())
            res += buf.sgetc();

        if (buf.sgetc() == '(')
        {
            for (; buf.sgetc() != ')'; buf.sbumpc())
                res += buf.sgetc();
            res += buf.sbumpc();
        }

        res += '=';
        for (; buf.sgetc() != '\n'; buf.sbumpc())
        {
            if (buf.sgetc() == '\\')
            {
                buf.sbumpc();
                if (buf.sgetc() == '\r')
                    buf.sbumpc();

                if (buf.sgetc() == '\n')
                    buf.sbumpc();
                else
                    res += '\\';
            }
            else if (buf.sgetc() == '/')
            {
                buf.sbumpc();
                if (buf.sgetc() == '/')
                    break; // C++ comment, goes until the end of the line
                else if (buf.sgetc() == '*')
                    break; // C style comment, macro could continue but ignore that
                else
                    res += '/'; // normal '/' character
            }
            else if (buf.sgetc() != '\r')
                res += buf.sgetc();
        }
        assert(res.back() != '\n');
        return res;
    }

    compile_config get_preprocess_config(CXTranslationUnit preprocess_tu, const char* full_path,
                                         compile_config c)
    {
        struct data_t
        {
            compile_config*   c;
            CXTranslationUnit tu;
            CXFile            file;
        } data{&c, preprocess_tu, clang_getFile(preprocess_tu, full_path)};

        clang_visitChildren(clang_getTranslationUnitCursor(preprocess_tu),
                            [](CXCursor cur, CXCursor, CXClientData cdata) -> CXChildVisitResult {
                                auto data = static_cast<data_t*>(cdata);

                                if (clang_getCursorKind(cur) == CXCursor_MacroDefinition)
                                {
                                    CXFile f;
                                    clang_getSpellingLocation(clang_getCursorLocation(cur), &f,
                                                              nullptr, nullptr, nullptr);
                                    if (!clang_File_isEqual(data->file, f))
                                    {
                                        // macro definition not in the main file,
                                        // make it available for libclang
                                        auto macro = get_macro(cur);
                                        if (!macro.empty())
                                            data->c->add_macro_definition(macro);
                                    }
                                }

                                return CXChildVisit_Continue;
                            },
                            &data);

        return c;
    }

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

    // get all macro definitions from libclang
    auto preprocess_tu     = get_cxunit_preprcoessing(index_.get(), c, full_path);
    auto preprocess_config = get_preprocess_config(preprocess_tu, full_path, c);
    clang_disposeTranslationUnit(preprocess_tu);

    // now preprocess source code and parse it
    auto source       = read_source(full_path);
    auto preprocessed = preprocessor_.preprocess(preprocess_config, full_path, source, *file_ptr);
    auto tu           = get_cxunit(index_.get(), c, full_path, preprocessed);
    parse_comments(*this, file_name, preprocessed);

    file_ptr->wrapper_ = detail::tu_wrapper(tu);
    file_ptr->set_cursor(clang_getTranslationUnitCursor(tu));

    return translation_unit(*this, full_path, file_ptr);
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
