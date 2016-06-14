// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_preprocessor.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>

using namespace standardese;

cpp_ptr<cpp_inclusion_directive> cpp_inclusion_directive::parse(translation_unit&, cpp_cursor cur,
                                                                const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_InclusionDirective);

    auto source = detail::tokenizer::read_source(cur);

    size_t i = 1u; // skip #
    while (std::isspace(source[i]))
        ++i;

    i += std::strlen("include");
    while (std::isspace(source[i]))
        ++i;

    auto k = source[i] == '<' ? kind::system : kind::local;

    return detail::make_ptr<cpp_inclusion_directive>(cur, parent, detail::parse_name(cur).c_str(),
                                                     k);
}

namespace
{
    void parse_macro(cpp_cursor cur, std::string& name, std::string& args, std::string& rep)
    {
        auto source = detail::tokenizer::read_source(cur);

        name   = detail::parse_name(cur).c_str();
        auto i = name.length();
        while (std::isspace(source[i]))
            ++i;

        // arguments
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
        while (i < source.size())
            rep += source[i++];
        detail::erase_trailing_ws(rep);
    }
}

cpp_ptr<cpp_macro_definition> cpp_macro_definition::parse(translation_unit&, cpp_cursor cur,
                                                          const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_MacroDefinition);

    std::string name, args, rep;
    parse_macro(cur, name, args, rep);

    return detail::make_ptr<cpp_macro_definition>(cur, parent, std::move(args), std::move(rep));
}

std::string detail::get_cmd_definition(cpp_cursor expansion_ref)
{
    std::string name, args, rep;
    parse_macro(clang_getCursorReferenced(expansion_ref), name, args, rep);

    return name + args + "=" + rep;
}
