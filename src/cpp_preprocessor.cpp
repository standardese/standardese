// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_preprocessor.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/search_token.hpp>

using namespace standardese;

cpp_ptr<cpp_inclusion_directive> cpp_inclusion_directive::parse(translation_unit &, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_InclusionDirective);

    kind k = local; // set to local because " isn't reached in the tokens
    auto found = false;
    detail::visit_tokens(cur, [&](CXToken, const string &spelling)
    {
        if (found)
        {
            if (spelling == "<")
                k = system;
            else if (spelling == "\"")
                k = local;

            return false;
        }
        else if (spelling == "include")
            found = true;

        return true;
    });

    return detail::make_ptr<cpp_inclusion_directive>(detail::parse_name(cur), detail::parse_comment(cur), k);
}

cpp_ptr<cpp_macro_definition> cpp_macro_definition::parse(translation_unit &, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_MacroDefinition);

    auto name = detail::parse_name(cur);
    std::string args;
    auto rep = detail::parse_macro_replacement(cur, name, args);

    return detail::make_ptr<cpp_macro_definition>(std::move(name), detail::parse_comment(cur),
                                                  std::move(args), std::move(rep));
}
