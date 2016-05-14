// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_preprocessor.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>

using namespace standardese;

cpp_ptr<cpp_inclusion_directive> cpp_inclusion_directive::parse(translation_unit &tu, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_InclusionDirective);

    detail::tokenizer tokenizer(tu, cur);
    auto& source = tokenizer.get_source();

    auto i = 1u; // skip #
    while (std::isspace(source[i]))
        ++i;

    i += std::strlen("include");
    while (std::isspace(source[i]))
        ++i;

    auto k = source[i] == '<' ? kind::system : kind::local;

    return detail::make_ptr<cpp_inclusion_directive>(detail::parse_name(cur), detail::parse_comment(cur), k);
}

cpp_ptr<cpp_macro_definition> cpp_macro_definition::parse(translation_unit &tu, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_MacroDefinition);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    auto name = detail::parse_name(cur);
    detail::skip(stream, {name.c_str()});

    // arguments
    std::string args;
    if (stream.peek().get_value() == "(")
    {
        args += "(";
        stream.bump();

        auto bracket_count = 1;
        while (bracket_count != 0)
        {
            auto spelling = stream.get().get_value();

            if (spelling == "(")
                ++bracket_count;
            else if (spelling == ")")
                --bracket_count;

            args += spelling.c_str();
        }
        detail::skip_whitespace(stream);

        while (std::isspace(args.back()))
            args.pop_back();
    }

    // replacement
    std::string rep;
    while (!stream.done())
        rep += stream.get().get_value().c_str();

    while (std::isspace(rep.back()))
        rep.pop_back();

    return detail::make_ptr<cpp_macro_definition>(std::move(name), detail::parse_comment(cur),
                                                  std::move(args), std::move(rep));
}
