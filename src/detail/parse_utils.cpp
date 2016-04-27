// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/search_token.hpp>

using namespace standardese;

cpp_name detail::parse_name(cpp_cursor cur)
{
    string str(clang_getCursorSpelling(cur));
    return cpp_name(str.get());
}

cpp_comment detail::parse_comment(cpp_cursor cur)
{
    string str(clang_Cursor_getRawCommentText(cur));
    return cpp_comment(str.get());
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

namespace
{
    bool is_identifier_char(char c) STANDARDESE_NOEXCEPT
    {
        return std::isalnum(c) || c == '_';
    }

    bool needs_whitespace(char a, char b) STANDARDESE_NOEXCEPT
    {
        if (is_identifier_char(a) && is_identifier_char(b))
            return true;
        else if (a == '(' || b == '('
                 || a == ')' || b == ')')
            // no whitespace for or after brackets
            return false;
        else if (a == '*' && b != '*')
            // to format "type *"
            return true;
        else if (a == ',')
            // to format "(a, b)"
            return true;
        return false;
    }

    void cat_token(cpp_name &result, const char *spelling)
    {
        if (needs_whitespace(result.back(), *spelling))
            result += ' ';
        result += spelling;
    }
}

cpp_name detail::parse_typedef_type_name(cpp_cursor cur, const cpp_name &name)
{
    cpp_name result;
    visit_tokens(cur, [&](CXToken, const string &spelling)
    {
        if (spelling == name.c_str() || spelling == "typedef")
            return true;

        cat_token(result, spelling);
        return true;
    });

    return result;
}

cpp_name detail::parse_variable_type_name(cpp_cursor cur, const cpp_name &name, std::string &initializer)
{
    cpp_name result;
    auto in_type = true;
    visit_tokens(cur, [&](CXToken, const string &spelling)
    {
        if (spelling == name.c_str()
          || spelling == "extern"
          || spelling == "static"
          || spelling == "thread_local")
            return true;
        else if (spelling == "=")
            in_type = false;
        else
            cat_token(in_type ? result : initializer, spelling);

        return true;
    });

    return result;
}

cpp_name detail::parse_alias_type_name(cpp_cursor cur)
{
    cpp_name result;
    auto found = false;
    visit_tokens(cur, [&](CXToken, const string &spelling)
    {
        if (!found && spelling == "=")
        {
            found = true;
            return true;
        }
        else if (!found)
            return true;

        cat_token(result, spelling);
        return true;
    });

    return result;
}

cpp_name detail::parse_enum_type_name(cpp_cursor cur)
{
    cpp_name result;
    auto found = false;
    visit_tokens(cur, [&](CXToken, const string &spelling)
    {
        if (!found && spelling == ":")
        {
            found = true;
            return true;
        }
        else if (found && spelling == "{")
            return false;
        else if (!found)
            return true;

        cat_token(result, spelling);
        return true;
    });

    return result;
}
