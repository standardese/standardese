// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_SEARCH_TOKEN_HPP_INCLUDED
#define STANDARDESE_DETAIL_SEARCH_TOKEN_HPP_INCLUDED

#include <clang-c/Index.h>
#include <string>

#include <standardese/string.hpp>

namespace standardese { namespace detail
{
    // calls fn for each token of a Cursor
    // gives token and spelling
    // aborts when returned false
    template <typename Fnc>
    void visit_tokens(CXCursor cur, Fnc fn)
    {
        auto tu = clang_Cursor_getTranslationUnit(cur);
        auto source = clang_getCursorExtent(cur);

        CXToken *tokens;
        unsigned no_tokens;
        clang_tokenize(tu, source, &tokens, &no_tokens);

        try
        {
            // don't use the last token, it doesn't really belong to cursor
            for (auto i = 0u; i != no_tokens - 1; ++i)
            {
                string str(clang_getTokenSpelling(tu, tokens[i]));
                auto res = fn(tokens[i], str);
                if (!res)
                    break;
            }
        }
        catch (...)
        {
            clang_disposeTokens(tu, tokens, no_tokens);
        }

        clang_disposeTokens(tu, tokens, no_tokens);
    }

    // searches for a token
    inline bool has_token(CXCursor cur, const char *token)
    {
        auto result = false;
        visit_tokens(cur, [&](CXToken, const string &spelling)
        {
            if (spelling == token)
            {
                result = true;
                return false;
            }

            return true;
        });

        return result;
    }

    // searches for a token that comes before name
    inline bool has_prefix_token(CXCursor cur, const char *token, const char *name)
    {
        auto result = false;
        visit_tokens(cur, [&](CXToken, const string &spelling)
        {
            if (spelling == token)
            {
               result = true;
               return false;
            }
            else if (spelling == name)
               return false;

            return true;
        });

        return result;
    }

    // searches for a token that comes after name
    inline bool has_suffix_token(CXCursor cur, const char *token, const char *name)
    {
        auto result = false;
        auto found = false;
        visit_tokens(cur, [&](CXToken, const string &spelling)
        {
            if (found && spelling == token)
                result = true;
            else if (!found && spelling == name)
                found = true;
            else if (found && spelling == name) // multiple occurence
            {
                // reset
                result = false;
                found = false;
            }

            return true;
        });

        return result;
    }

    // concatenates all tokens for a cursor
    inline std::string cat_tokens(CXCursor cur)
    {
        std::string result;

        visit_tokens(cur, [&](CXToken, const string &spelling)
        {
            result += spelling;
            return true;
        });

        return result;
    }

    // returns all tokens after a certain token
    inline std::string cat_tokens_after(CXCursor cur, const char *token, const char *until = "")
    {
        std::string result;
        auto found = false;
        visit_tokens(cur, [&](CXToken, const string &spelling)
        {
            if (!found && spelling == token)
                found = true;
            else if (found && spelling == until)
                return false;
            else if (found)
                result += spelling;

            return true;
        });

        return result;
    }
}} // namespace standardese::detail

#endif // STANDARDESE_DETAIL_SEARCH_TOKEN_HPP_INCLUDED
