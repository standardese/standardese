// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/search_token.hpp>
#include <standardese/cpp_function.hpp>

#include <cassert>

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
        else if ((a != '*' && b == '*') || (a != '&' && b == '&'))
            // to format "type *"
            return true;
        else if (a == ',')
            // to format "(a, b)"
            return true;
        return false;
    }

    void cat_token(cpp_name &result, const char *spelling)
    {
        if (!result.empty() && needs_whitespace(result.back(), *spelling))
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

cpp_name detail::parse_function_info(cpp_cursor cur, const cpp_name &name,
                                     int &function_flags, std::string &noexcept_expr)
{
    cpp_name result;
    auto bracket_count = 0;

    enum
    {
        normal_return,
        auto_return,
        decltype_return
    } ret = normal_return;

    enum
    {
        return_type,
        parameters,
        noexcept_expression
    } state = return_type;

    auto was_noexcept = false;
    noexcept_expr.clear();

    visit_tokens(cur, [&](CXToken, const string &spelling)
    {
        if (state == return_type)
        {
            if (spelling == "extern"
                || spelling == "static"
                || spelling == "virtual"
                || spelling == "explicit")
                return true; // skip leading ignored keywords
            else if (spelling == "constexpr")
                function_flags |= cpp_constexpr_fnc; // add constepxr flag
            else if (spelling == "noexcept")
            {
                state = noexcept_expression; // enter noexcept expression
                was_noexcept = true;
            }
            else if (ret != decltype_return && spelling == "auto")
                ret = auto_return; // mark auto return type
            else if (spelling == name.c_str())
                state = parameters; // enter paramaters
            else if (spelling == ";" || spelling == "{")
                return false; // finish with header
            else
            {
                if (spelling == "decltype")
                    ret = decltype_return; // decltype return, allow auto
                cat_token(result, spelling); // part of return type
            }
        }
        else if (state == parameters)
        {
            if (spelling == "(")
                bracket_count++;
            else if (spelling == ")")
                bracket_count--;

            if (bracket_count == 0)
                state = return_type;
        }
        else if (state == noexcept_expression)
        {
            if (bracket_count > 0 && (bracket_count != 1 || spelling != ")"))
                // if inside the noexcept(...)
                cat_token(noexcept_expr, spelling);

            if (spelling == "(")
                bracket_count++;
            else if (spelling == ")")
                bracket_count--;

            if (bracket_count == 0)
                state = return_type;
        }
        else
            assert(false);

        return true;
    });

    if (ret == auto_return && !result.empty())
        // trailing return type, erase "->"
        result.erase(0, 2);
    else if (ret == auto_return)
        // deduced return type
        result = "auto";

    if (was_noexcept && noexcept_expr.empty())
        // this means simply noexcept without a condition
        noexcept_expr = "true";
    else if (!was_noexcept)
        // no noexcept
        noexcept_expr = "false";

    return result;
}
