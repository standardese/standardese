// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_preprocessor.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>

using namespace standardese;

cpp_ptr<cpp_inclusion_directive> cpp_inclusion_directive::parse(translation_unit &, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_InclusionDirective);

    auto source = detail::tokenizer::read_source(cur);

    auto i = 1u; // skip #
    while (std::isspace(source[i]))
        ++i;

    i += std::strlen("include");
    while (std::isspace(source[i]))
        ++i;

    auto k = source[i] == '<' ? kind::system : kind::local;

    return detail::make_ptr<cpp_inclusion_directive>(detail::parse_name(cur), k);
}

cpp_ptr<cpp_macro_definition> cpp_macro_definition::parse(translation_unit &, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_MacroDefinition);

    auto source = detail::tokenizer::read_source(cur);

    auto name = detail::parse_name(cur);
    auto i = name.size();
    while (std::isspace(source[i]))
        ++i;

    // arguments
    std::string args;
    if (source[i] == '(')
    {
        args += "(";
        ++i;

        auto bracket_count = 1;
        while (bracket_count != 0)
        {
            auto spelling = source[i];
            ++i;

            if (spelling == '(')
                ++bracket_count;
            else if (spelling == ')')
                --bracket_count;

            args += spelling;
        }

        while (std::isspace(source[i]))
            ++i;
    }

    // replacement
    std::string rep;
    while (i < source.size())
        rep += source[i++];

    while (std::isspace(rep.back()))
        rep.pop_back();

    return detail::make_ptr<cpp_macro_definition>(std::move(name),
                                                  std::move(args), std::move(rep));
}
