// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_preprocessor.hpp>

#include <cassert>
#include <fstream>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>

using namespace standardese;

cpp_ptr<cpp_inclusion_directive> cpp_inclusion_directive::parse(translation_unit&, cpp_cursor cur,
                                                                const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_InclusionDirective);

    return detail::make_cpp_ptr<cpp_inclusion_directive>(cur, parent,
                                                         detail::parse_name(cur).c_str());
}

namespace
{
    // returns true if macro is predefined
    void parse_macro(const std::string& source, const std::string& name, std::string& args,
                     std::string& rep)
    {
        auto i = name.length();
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
        else
            // skip whitespace
            while (std::isspace(source[i]))
                ++i;

        // replacement
        while (i < source.size())
            rep += source[i++];
        detail::erase_trailing_ws(rep);
    }

    CXFile get_range(cpp_cursor cur, unsigned& begin_offset, unsigned& end_offset)
    {
        auto source = clang_getCursorExtent(cur);
        auto begin  = clang_getRangeStart(source);
        auto end    = clang_getRangeEnd(source);

        // translate location into offset and file
        CXFile file  = nullptr;
        begin_offset = 0u;
        end_offset   = 0u;
        clang_getSpellingLocation(begin, &file, nullptr, nullptr, &begin_offset);
        clang_getSpellingLocation(end, nullptr, nullptr, nullptr, &end_offset);

        return file;
    }

    std::string get_source(cpp_cursor cur)
    {
        unsigned begin, end;
        auto     file = get_range(cur, begin, end);
        if (!file)
            return "";

        auto path = string(clang_getFileName(file));

        // open file in binary mode
        std::filebuf filebuf;
        filebuf.open(path.c_str(), std::ios_base::in | std::ios_base::binary);
        assert(filebuf.is_open());

        // seek to begin
        filebuf.pubseekpos(begin);

        // read bytes
        std::string result(end - begin, '\0');
        filebuf.sgetn(&result[0], result.size());

        return result;
    }

    bool parse_macro(cpp_cursor cur, std::string& name, std::string& args, std::string& rep)
    {
        name = detail::parse_name(cur).c_str();

        auto source = get_source(cur);
        if (source.empty())
            // predefined macro, cannot parse
            return true;

        parse_macro(source, name, args, rep);
        return false;
    }
}

cpp_ptr<cpp_macro_definition> cpp_macro_definition::parse(translation_unit&, cpp_cursor cur,
                                                          const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_MacroDefinition);

    std::string name, args, rep;
    auto        predefined = parse_macro(cur, name, args, rep);
    assert(!predefined);
    (void)predefined;

    return detail::make_cpp_ptr<cpp_macro_definition>(cur, parent, std::move(args), std::move(rep));
}

std::string detail::get_cmd_definition(translation_unit&, cpp_cursor macro)
{
    std::string name, args, rep;
    parse_macro(macro, name, args, rep);

    return name + args + "=" + rep;
}
