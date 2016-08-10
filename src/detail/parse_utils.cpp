// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/parse_utils.hpp>
#include <standardese/cpp_entity_registry.hpp>

#include <cassert>
#include <cctype>

using namespace standardese;

cpp_name detail::parse_scope(cpp_cursor cur)
{
    std::string result;
    cur = clang_getCursorSemanticParent(cur);
    while (!clang_isInvalid(clang_getCursorKind(cur))
           && !clang_isTranslationUnit(clang_getCursorKind(cur)))
    {
        auto str = detail::parse_name(cur);
        if (result.empty())
            result = str.c_str();
        else
            result = std::string(str.c_str()) + "::" + result;
        cur        = clang_getCursorSemanticParent(cur);
    }

    return result;
}

cpp_name detail::parse_name(cpp_cursor cur)
{
    return cpp_name(clang_getCursorSpelling(cur));
}

cpp_name detail::parse_name(CXType type)
{
    return cpp_name(clang_getTypeSpelling(type));
}

string detail::parse_comment(cpp_cursor cur)
{
    return string(clang_Cursor_getRawCommentText(cur));
}

cpp_name detail::parse_class_name(cpp_cursor cur)
{
    std::string name = parse_name(cur).c_str();

    auto pos = name.find(' ');
    if (pos == std::string::npos)
        return name;
    else if (name.compare(0, pos, "class") == 0 || name.compare(0, pos, "struct") == 0
             || name.compare(0, pos, "union") == 0)
        return name.substr(pos + 1);
    return name;
}

void detail::erase_template_args(std::string& name)
{
    auto beg = name.find('<');
    if (beg == std::string::npos)
        return;

    auto end   = name.rfind('>');
    auto count = end - beg + 1;

    assert(end != std::string::npos);
    name.erase(beg, count);
}

void detail::erase_trailing_ws(std::string& name)
{
    while (!name.empty() && std::isspace(name.back()))
        name.pop_back();
}
