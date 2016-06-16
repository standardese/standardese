// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/tokenizer.hpp>

#include <cassert>
#include <fstream>
#include <string>

#include <boost/version.hpp>

#include <standardese/cpp_cursor.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

#if (BOOST_VERSION / 100000) != 1
#error "require Boost 1.x"
#endif

#if ((BOOST_VERSION / 100) % 1000) < 55
#warning "Boost less than 1.55 isn't tested"
#endif

void detail::skip_whitespace(token_stream& stream)
{
    while (std::isspace(stream.peek().get_value()[0]))
        stream.bump();
}

void detail::skip(token_stream& stream, const cpp_cursor& cur, const char* value)
{
    auto& val = stream.peek();
    if (val.get_value() != value)
        throw parse_error(source_location(cur), std::string("expected \'") + value + "\' got \'"
                                                    + val.get_value().c_str() + "\'");
    stream.bump();
}

void detail::skip(token_stream& stream, const cpp_cursor& cur,
                  std::initializer_list<const char*> values)
{
    for (auto val : values)
    {
        skip(stream, cur, val);
        skip_whitespace(stream);
    }
}

bool detail::skip_if_token(detail::token_stream& stream, const char* token)
{
    if (stream.peek().get_value() != token)
        return false;
    stream.bump();
    detail::skip_whitespace(stream);
    return true;
}

void detail::skip_attribute(detail::token_stream& stream, const cpp_cursor& cur)
{
    if (stream.peek().get_value() == "[" && stream.peek(1).get_value() == "[")
    {
        stream.bump(); // opening
        skip_bracket_count(stream, cur, "[", "]");
        stream.bump(); // closing
    }
    else if (skip_if_token(stream, "__attribute__"))
    {
        skip(stream, cur, "(");
        skip_bracket_count(stream, cur, "(", ")");
        skip(stream, cur, ")");
    }
}

std::string detail::tokenizer::read_source(cpp_cursor cur)
{
    auto source = clang_getCursorExtent(cur);
    auto begin  = clang_getRangeStart(source);
    auto end    = clang_getRangeEnd(source);

    // translate location into offset and file
    CXFile   file = nullptr;
    unsigned begin_offset = 0u, end_offset = 0u;
    clang_getSpellingLocation(begin, &file, nullptr, nullptr, &begin_offset);
    clang_getSpellingLocation(end, nullptr, nullptr, nullptr, &end_offset);
    
    if (!file)
        return "";
    
    assert(end_offset > begin_offset);

    // open file buffer
    std::filebuf buf;

    string filename(clang_getFileName(file));
    buf.open(filename.c_str(), std::ios_base::in | std::ios_base::binary);
    assert(buf.is_open());

    // seek to beginning
    buf.pubseekpos(begin_offset);

    // read bytes
    std::string result(end_offset - begin_offset, '\0');
    buf.sgetn(&result[0], result.size());

    // awesome libclang bug:
    // if there is a macro expansion at the end, the closing bracket is missing
    // ie.: using foo = IMPL_DEFINED(bar
    // go backwards, if a '(' is found before a ')', append a ')'
    if (clang_getCursorKind(cur) == CXCursor_MacroDefinition)
        for (auto iter = result.rbegin(); iter != result.rend(); ++iter)
        {
            if (*iter == ')')
                break;
            else if (*iter == '(')
            {
                result += ')';
                break;
            }
        }

    // awesome libclang bug 2.0:
    // the extent of a function cursor doesn't cover any "= delete"
    // so append all further characters until a ';' is reached
    auto is_function = clang_getCursorKind(cur) == CXCursor_FunctionDecl
                       || clang_getTemplateCursorKind(cur) == CXCursor_FunctionDecl;
    auto needs_definition = result.back() != ';' && result.back() != '}';
    if (is_function && needs_definition)
    {
        while (buf.sgetc() != ';')
        {
            result += buf.sgetc();
            buf.sbumpc();
        }
    }

    // awesome libclang bug 3.0
    // the extent for a type alias is too short
    // needs to be extended until the semicolon
    if (clang_getCursorKind(cur) == CXCursor_TypeAliasDecl && result.back() != ';')
        while (buf.sgetc() != ';')
        {
            result += buf.sgetc();
            buf.sbumpc();
        }

    // prevent more awesome libclang bugs:
    // read until next line, just to be somewhat sure
    if (clang_getCursorKind(cur) != CXCursor_ParmDecl
        && clang_getCursorKind(cur) != CXCursor_TemplateTypeParameter
        && clang_getCursorKind(cur) != CXCursor_NonTypeTemplateParameter
        && clang_getCursorKind(cur) != CXCursor_TemplateTemplateParameter
        && clang_getCursorKind(cur) != CXCursor_CXXBaseSpecifier)
        while (buf.sgetc() != '\n')
        {
            result += buf.sgetc();
            buf.sbumpc();
        }

    return result;
}

namespace standardese
{
    namespace detail
    {
        context& get_preprocessing_context(translation_unit& tu);
    }
} // namespace standardese::detail

detail::tokenizer::tokenizer(translation_unit& tu, cpp_cursor cur)
: source_(read_source(cur)), impl_(&get_preprocessing_context(tu))
{
    // append trailing newline
    // required for parsing code
    source_ += '\n';
}
