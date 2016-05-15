// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/parse_utils.hpp>
#include <standardese/cpp_function.hpp>
#include <standardese/string.hpp>

#include <cassert>

using namespace standardese;

void detail::clean_name(cpp_name &name)
{
    auto beg = name.find('<');
    if (beg == cpp_name::npos)
        return;

    auto end = name.rfind('>');
    auto count = end - beg + 1;

    assert(end != cpp_name::npos);
    name.erase(beg, count);
}

cpp_name detail::parse_name(cpp_cursor cur)
{
    string str(clang_getCursorSpelling(cur));
    return cpp_name(str.get());
}

cpp_name detail::parse_name(CXType type)
{
    string str(clang_getTypeSpelling(type));
    return cpp_name(str.get());
}

cpp_name detail::parse_class_name(cpp_cursor cur)
{
    auto name = parse_name(cur);
    auto pos = name.find(' ');
    return name.substr(pos + 1);
}

cpp_raw_comment detail::parse_comment(cpp_cursor cur)
{
    string str(clang_Cursor_getRawCommentText(cur));
    return cpp_raw_comment(str.get());
}

cpp_name detail::parse_scope(cpp_cursor cur)
{
    cpp_name result;
    cur = clang_getCursorSemanticParent(cur);
    while (!clang_isInvalid(clang_getCursorKind(cur)) && !clang_isTranslationUnit(clang_getCursorKind(cur)))
    {
        auto str = detail::parse_name(cur);
        if (result.empty())
            result = str;
        else
            result = str + "::" + result;
        cur = clang_getCursorSemanticParent(cur);
    }
    return result;
}

// when concatenating tokens for the default values
// template <typename T = foo<int>> yields foo<int>>
// because >> is one token
// count brackets, if unbalanced, remove final >
void detail::unmunch(std::string &str)
{
    auto balance = 0;
    for (auto c : str)
        if (c == '<')
            balance++;
        else if (c == '>')
            balance--;

    assert(balance == 0 || balance == -1);
    if (balance == -1)
    {
        assert(str.back() == '>');
        str.pop_back();
    }
}
