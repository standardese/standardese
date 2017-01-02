// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/parser.hpp>

#include <standardese/detail/tokenizer.hpp>
#include <standardese/cpp_preprocessor.hpp>
#include <standardese/error.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

namespace
{
    unsigned get_diagnostic_options()
    {
        return CXDiagnostic_DisplayOption;
    }

    CXTranslationUnit get_cxunit(const std::shared_ptr<spdlog::logger>& log, CXIndex index,
                                 const compile_config& c, const char* full_path,
                                 const std::string& source)
    {
        auto args = c.get_flags();
        // allow detection of friend definitions
        args.push_back("-D__standardese_friend=static");

        CXUnsavedFile file;
        file.Filename = full_path;
        file.Contents = source.c_str();
        file.Length   = source.length();

        CXTranslationUnit tu;
        auto              error = clang_parseTranslationUnit2(index, full_path, args.data(),
                                                 static_cast<int>(args.size()), &file, 1,
                                                 CXTranslationUnit_Incomplete
#if CINDEX_VERSION_MINOR >= 34
                                                     | CXTranslationUnit_KeepGoing
#endif
                                                 ,
                                                 &tu);
        if (error != CXError_Success)
            throw libclang_error(error, "CXTranslationUnit (" + std::string(full_path) + ")");

        auto no_diagnostics = clang_getNumDiagnostics(tu);
        for (auto i = 0u; i != no_diagnostics; ++i)
        {
            auto diag = clang_getDiagnostic(tu, i);

            if (clang_getDiagnosticSeverity(diag) > CXDiagnostic_Warning)
            {
                auto msg = string(clang_formatDiagnostic(diag, get_diagnostic_options()));
                if (!std::strstr(msg.c_str(), "cannot be a static member function"))
                    log->error("[compiler] {}", msg.c_str());
            }

            clang_disposeDiagnostic(diag);
        }

        return tu;
    }

    std::string replace_friend_definitions(const std::string& preprocessed)
    {
        std::string output;

        auto last_match = preprocessed.c_str();
        while (const char* match = std::strstr(last_match, "friend"))
        {
            output.append(last_match, match);

            auto ptr = match;
            for (auto paren_count = 0; *ptr && *ptr != ';'; ++ptr)
            {
                if (*ptr == '(')
                    ++paren_count;
                else if (*ptr == ')')
                    --paren_count;
                else if (paren_count == 0 && *ptr == '{')
                    // friend function definition
                    break;
            }

            last_match = ptr;
            if (!*ptr || *ptr == ';')
                // other friend, keep
                output.append(match, ptr);
            else
            {
                // friend function definition, replace keyword
                output += "__standardese_friend";
                match += std::strlen("friend");
                output.append(match, ptr);
            }
        }
        output.append(last_match, &preprocessed.back() + 1);
        return output;
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
    auto tu =
        get_cxunit(logger_, index_.get(), c, full_path, replace_friend_definitions(preprocessed));
    parse_comments(*this, file_name, preprocessed);

    file_ptr->wrapper_ = detail::tu_wrapper(tu);
    file_ptr->set_cursor(clang_getTranslationUnitCursor(tu));

    return translation_unit(*this, full_path, file_ptr);
}

parser::parser(std::shared_ptr<spdlog::logger> logger)
: index_(clang_createIndex(1, 0)), logger_(std::move(logger))
{
}

parser::~parser() STANDARDESE_NOEXCEPT
{
}

void parser::deleter::operator()(CXIndex idx) const STANDARDESE_NOEXCEPT
{
    clang_disposeIndex(idx);
}
