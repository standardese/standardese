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

cpp_name detail::parse_class_name(cpp_cursor cur)
{
    auto name = parse_name(cur);
    auto pos = name.find(' ');
    return name.substr(pos + 1);
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

cpp_name detail::parse_enum_type_name(cpp_cursor cur, bool &definition)
{
    definition = false;

    cpp_name result;
    auto found = false;
    visit_tokens(cur, [&](CXToken, const string &spelling)
    {
        if (!found && spelling == ":")
        {
            found = true;
            return true;
        }
        else if (spelling == "{")
        {
            definition = true;
            return false;
        }
        else if (spelling == ";")
            return false;
        else if (!found)
            return true;

        cat_token(result, spelling);
        return true;
    });

    return result;
}

namespace
{
    // compares until whitespace
    bool token_equal(const string &token, const char* &ptr)
    {
        auto save = ptr;
        auto token_ptr = token.get();
        while (true)
        {
            if (!*token_ptr)
                return true;
            else if (!*ptr)
                break;
            else if (*ptr == ' ')
                return true;
            else if (*token_ptr != *ptr)
                break;
            ++token_ptr;
            ++ptr;
        }
        ptr = save;
        return false;
    }
}

cpp_name detail::parse_function_info(cpp_cursor cur, const cpp_name &name,
                                     cpp_function_info &finfo,
                                     cpp_member_function_info &minfo)
{
    cpp_name result;
    auto bracket_count = 0;
    auto start_parameters = 0;

    auto ptr = name.c_str();

    enum
    {
        normal_return,
        auto_return,
        decltype_return
    } ret = normal_return;

    enum
    {
        prefix,
        parameters,
        suffix,
        noexcept_expression,
        trailing_return,
        definition,
    } state = prefix;

    auto was_noexcept = false;
    finfo.noexcept_expression.clear();

    visit_tokens(cur, [&](CXToken, const string &spelling)
    {
        if (spelling == "(")
            bracket_count++;
        else if (spelling == ")")
            bracket_count--;

        if (spelling != ")" && bracket_count == 0u && state == noexcept_expression)
            // no brackets, directly back
            state = suffix;

        if (state == prefix) // everything before the function name
        {
            if (spelling == "extern"
                || spelling == "static")
                return true; // skip leading ignored keywords
            else if (spelling == "operator")
            {
                assert(name.compare(0, 8, "operator") == 0);
                ptr += 8; // bump pointer for comparision
                while (*ptr == ' ')
                    ++ptr;
            }
            else if (spelling == "constexpr")
                finfo.set_flag(cpp_constexpr_fnc); // add constepxr flag
            else if (spelling == "explicit")
                finfo.set_flag(cpp_explicit_conversion); // add explicit flag
            else if (spelling == "virtual")
                minfo.virtual_flag = cpp_virtual_new; // mark virtual
            else if (ret != decltype_return && spelling == "auto")
                ret = auto_return; // mark auto return type
            // parameter begin
            else if (token_equal(spelling, ptr)) // consume only up to whitespace for conversion op
            {
                if (!*ptr)
                {
                    state = parameters; // enter parameters
                    start_parameters = bracket_count;
                }
            }
            // normal case
            else
            {
                if (spelling == "decltype")
                    ret = decltype_return; // decltype return, allow auto
                cat_token(result, spelling); // part of return type
            }
        }
        else if (state == parameters) // parameter part
        {
            if (bracket_count == start_parameters)
                state = suffix; // go to suffix if outside
        }
        else if (state == suffix) // rest of return type, other keywords at the end
        {
            // first handle state switch conditions
            if (spelling == "->")
            {
                state = trailing_return; // trailing return type
                return true; // consume token
            }
            else if (spelling == ";" || spelling == "{")
                return false; // finish with declaration part
            else if (spelling == "=")
            {
                state = definition; // enter definition
                return true;
            }
            else if (spelling == "noexcept")
            {
                state = noexcept_expression; // enter noexcept expression
                was_noexcept = true;
                return true;
            }

            // count brackets to handle: int (*f(int a))(volatile char);
            // i.e. don't mistake the cv there for a cv specifier
            // outside of the parameters of a function ptr return type
            if (bracket_count == 0)
            {
                if (spelling == "const")
                    minfo.set_cv(cpp_cv_const); // const member function
                else if (spelling == "volatile")
                    minfo.set_cv(cpp_cv_volatile); // volatile member function
                else if (spelling == "&")
                    minfo.ref_qualifier = cpp_ref_lvalue; // lvalue member function
                else if (spelling == "&&")
                    minfo.ref_qualifier = cpp_ref_rvalue; // rvalue member function
                else if (spelling == "override")
                    minfo.virtual_flag = cpp_virtual_overriden; // make override
                else if (spelling == "final")
                    minfo.virtual_flag = cpp_virtual_final; // make final
                else
                    cat_token(result, spelling); // part of return type
            }
            else
                cat_token(result, spelling); // part of return type
        }
        else if (state == noexcept_expression) // handles the (...) part of noexcept(...)
        {
            if (bracket_count > 0 && (bracket_count != 1 || spelling != "("))
                // if inside the noexcept(...)
                cat_token(finfo.noexcept_expression, spelling);
            else if (bracket_count == 0)
            {
                assert(spelling == ")");
                state = suffix; // continue with suffix
            }
        }
        else if (state == trailing_return) // trailing return type
        {
            if (spelling == "=")
                state = definition; // enter definition
            else if (spelling == ";" || spelling == "{")
                return false; // finished with body
            else
                cat_token(result, spelling); // part of return type
        }
        else if (state == definition) // deleted, defaulted, pure virtual
        {
            if (spelling == "delete")
                finfo.definition = cpp_function_definition_deleted;
            else if (spelling == "default")
                finfo.definition = cpp_function_definition_defaulted;
            else if (spelling == "0")
                minfo.virtual_flag = cpp_virtual_pure; // make pure virtual
            else
                assert(false);
            return false; // nothing comes after this
        }
        else
            assert(false);

        return true;
    });

    if (ret == auto_return && result.empty())
        // deduced return type
        result = "auto";

    if (was_noexcept && finfo.noexcept_expression.empty())
        // this means simply noexcept without a condition
        finfo.noexcept_expression = "true";

    // set variadic flag
    if (clang_isFunctionTypeVariadic(clang_getCursorType(cur)))
        finfo.set_flag(cpp_variadic_fnc);

    return result;
}
