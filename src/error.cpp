// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/error.hpp>

#include <standardese/cpp_cursor.hpp>
#include <standardese/string.hpp>

using namespace standardese;

namespace
{
    const char* to_string(CXErrorCode error)
    {
        switch (error)
        {
        case CXError_Success:
            return "success";
        case CXError_ASTReadError:
            return "AST read error/parsing error";
        case CXError_Crashed:
            return "libclang crashed";
        case CXError_InvalidArguments:
            return "invalid arguments passed to function";
        case CXError_Failure:
            return "generic failure";
        }

        return "generic generic failure";
    }
}

libclang_error::libclang_error(CXErrorCode error, std::string type)
: std::runtime_error(std::string(type) + ": " + to_string(error))
{
}

source_location::source_location(CXSourceLocation location, std::string entity)
: entity_name(std::move(entity))
{
    CXFile file;
    clang_getSpellingLocation(location, &file, &line, nullptr, nullptr);
    file_name = string(clang_getFileName(file)).c_str();
}

source_location::source_location(cpp_cursor cur)
: source_location(clang_getCursorLocation(cur), string(clang_getCursorDisplayName(cur)).c_str())
{
}
