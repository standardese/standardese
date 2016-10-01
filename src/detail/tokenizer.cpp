// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/tokenizer.hpp>

#include <standardese/error.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

namespace
{
    unsigned get_token_offset(CXTranslationUnit tu, CXToken token)
    {
        unsigned offset;
        auto     location = clang_getTokenLocation(tu, token);
        clang_getSpellingLocation(location, nullptr, nullptr, nullptr, &offset);
        return offset;
    }
}

detail::token::token(CXTranslationUnit tu, CXToken token)
: detail::token(clang_getTokenSpelling(tu, token), clang_getTokenKind(token),
                get_token_offset(tu, token))
{
}

namespace
{
    bool cursor_is_function(CXCursorKind kind)
    {
        return kind == CXCursor_FunctionDecl || kind == CXCursor_CXXMethod
               || kind == CXCursor_Constructor || kind == CXCursor_Destructor
               || kind == CXCursor_ConversionFunction;
    }

    CXSourceLocation get_next_location(CXTranslationUnit tu, CXFile file, CXSourceLocation loc,
                                       int inc = 1)
    {
        unsigned offset;
        clang_getSpellingLocation(loc, nullptr, nullptr, nullptr, &offset);
        return clang_getLocationForOffset(tu, file, offset + inc);
    }

    string get_token_after(CXTranslationUnit tu, CXFile file, CXSourceLocation loc)
    {
        auto loc_after = get_next_location(tu, file, loc);

        CXToken* token;
        unsigned no;
        clang_tokenize(tu, clang_getRange(loc, loc_after), &token, &no);

        assert(no >= 1);
        string spelling(clang_getTokenSpelling(tu, token[0]));

        clang_disposeTokens(tu, token, no);
        return spelling;
    }

    CXSourceRange get_extent(CXTranslationUnit tu, CXFile file, cpp_cursor cur,
                             unsigned& end_offset, const char*& end_token)
    {
        end_token = ";";

        auto extent       = clang_getCursorExtent(cur);
        auto begin        = clang_getRangeStart(extent);
        auto end          = clang_getRangeEnd(extent);
        auto range_shrunk = false;

        if (cursor_is_function(clang_getCursorKind(cur))
            || cursor_is_function(clang_getTemplateCursorKind(cur)))
        {
            // if a function we need to remove the body
            // it does not need to be parsed
            detail::visit_children(cur, [&](cpp_cursor child, cpp_cursor) {
                if (clang_getCursorKind(child) == CXCursor_CompoundStmt
                    || clang_getCursorKind(child) == CXCursor_CXXTryStmt
                    || clang_getCursorKind(child) == CXCursor_InitListExpr)
                {
                    auto child_extent = clang_getCursorExtent(child);
                    end               = clang_getRangeStart(child_extent);
                    range_shrunk      = true;
                    end_token         = "{";
                    return CXChildVisit_Break;
                }
                return CXChildVisit_Continue;
            });

            if (!range_shrunk && get_token_after(tu, file, end) != ";")
            {
                // we do not have a body, but it is not a declaration either
                do
                {
                    end = get_next_location(tu, file, end);
                } while (get_token_after(tu, file, end) != ";");
            }
        }
        else if (clang_getCursorKind(cur) == CXCursor_TemplateTypeParameter
                 || clang_getCursorKind(cur) == CXCursor_NonTypeTemplateParameter
                 || clang_getCursorKind(cur) == CXCursor_TemplateTemplateParameter
                 || clang_getCursorKind(cur) == CXCursor_ParmDecl)
        {
            if (clang_getCursorKind(cur) == CXCursor_TemplateTypeParameter
                && get_token_after(tu, file, end) == "(")
            {
                // if you have decltype as default argument for a type template parameter
                // libclang doesn't include the parameters
                auto next = get_next_location(tu, file, end);
                auto prev = end;
                for (auto paren_count = 1; paren_count != 0;
                     next             = get_next_location(tu, file, next))
                {
                    auto spelling = get_token_after(tu, file, next);
                    if (spelling == "(")
                        ++paren_count;
                    else if (spelling == ")")
                        --paren_count;
                    prev = next;
                }
                end = prev;
            }
            else
                // range includes the following token ('>'/')' or ',') for (template) parameters
                // don't need that
                range_shrunk = true;
        }
        else if (clang_getCursorKind(cur) == CXCursor_TypeAliasDecl
                 && get_token_after(tu, file, end) != ";")
        {
            // type alias tokens don't include everything
            do
            {
                end = get_next_location(tu, file, end);
            } while (get_token_after(tu, file, end) != ";");
            end = get_next_location(tu, file, end);
        }

        clang_getSpellingLocation(end, nullptr, nullptr, nullptr, &end_offset);
        if (range_shrunk)
            --end_offset; // token range is inclusive, but don't need last token

        return clang_getRange(begin, end);
    }

    CXSourceRange get_extent(CXTranslationUnit tu, CXFile file, cpp_cursor cur)
    {
        unsigned    unused;
        const char* unused2;
        return get_extent(tu, file, cur, unused, unused2);
    }
}

CXFile detail::get_range(const translation_unit& tu, cpp_cursor cur, unsigned& begin_offset,
                         unsigned& end_offset)
{
    auto source = get_extent(tu.get_cxunit(), tu.get_cxfile(), cur);
    return get_range(source, begin_offset, end_offset);
}

CXFile detail::get_range(CXSourceRange extent, unsigned& begin_offset, unsigned& end_offset)
{
    auto begin = clang_getRangeStart(extent);
    auto end   = clang_getRangeEnd(extent);

    // translate location into offset and file
    CXFile file  = nullptr;
    begin_offset = 0u;
    end_offset   = 0u;
    clang_getSpellingLocation(begin, &file, nullptr, nullptr, &begin_offset);
    clang_getSpellingLocation(end, nullptr, nullptr, nullptr, &end_offset);

    return file;
}

detail::tokenizer::tokenizer(CXTranslationUnit tu, CXFile file, cpp_cursor cur)
: tu_(tu), file_(file)
{
    clang_tokenize(get_cxunit(), get_extent(get_cxunit(), get_cxfile(), cur, end_offset_, end_),
                   &tokens_, &no_tokens_);
    assert(tokens_);
}

detail::tokenizer::~tokenizer() STANDARDESE_NOEXCEPT
{
    clang_disposeTokens(get_cxunit(), tokens_, no_tokens_);
}

namespace
{
    CXToken* get_actual_end(CXTranslationUnit tu, CXToken* tokens, unsigned no_tokens,
                            unsigned end_offset)
    {
        // need to find the last token that is actually part of the cursor
        // libclang is always good with the extent of cursors
        auto end = tokens + no_tokens;
        while (get_token_offset(tu, end[-1]) > end_offset)
        {
            assert(end > tokens);
            --end;
        }

        return end;
    }
}

detail::token_iterator detail::tokenizer::end() const STANDARDESE_NOEXCEPT
{
    auto end = get_actual_end(get_cxunit(), tokens_, no_tokens_, end_offset_);
    return token_iterator(get_cxunit(), end);
}

namespace
{
    CXSourceRange get_following_range(CXTranslationUnit tu, CXFile file, unsigned end_offset)
    {
        auto begin = clang_getLocationForOffset(tu, file, end_offset + 1);
        auto end   = clang_getLocationForOffset(tu, file, end_offset + 2);
        return clang_getRange(begin, end);
    }
}

bool detail::tokenizer::need_unmunch() const STANDARDESE_NOEXCEPT
{
    CXToken* next_tokens;
    unsigned no;
    clang_tokenize(get_cxunit(), get_following_range(get_cxunit(), get_cxfile(), end_offset_),
                   &next_tokens, &no);

    assert(no >= 1);
    auto next_spelling = string(clang_getTokenSpelling(get_cxunit(), *next_tokens));
    auto valid_token   = next_spelling == ">"
                       || next_spelling == ","; // must be comma or end of template argument list

    clang_disposeTokens(get_cxunit(), next_tokens, no);
    return !valid_token;
}

void detail::skip_offset(detail::token_stream& stream, unsigned offset)
{
    while (stream.peek().get_offset() < offset)
        stream.bump();
}

void detail::skip(token_stream& stream, const cpp_cursor& cur, const char* value)
{
    if (!*value)
        return;

    auto val = stream.peek();
    if (val.get_value() != value)
        throw parse_error(source_location(cur), std::string("expected \'") + value + "\' got \'"
                                                    + val.get_value().c_str() + "\'");
    stream.bump();
}

void detail::skip(token_stream& stream, const cpp_cursor& cur,
                  std::initializer_list<const char*> values)
{
    for (auto val : values)
        skip(stream, cur, val);
}

bool detail::skip_if_token(detail::token_stream& stream, const char* token)
{
    if (stream.peek().get_value() != token)
        return false;
    stream.bump();
    return true;
}

bool detail::skip_attribute(detail::token_stream& stream, const cpp_cursor& cur)
{
    if (stream.peek().get_value() == "[" && stream.peek(1).get_value() == "[")
    {
        stream.bump(); // opening
        skip_bracket_count(stream, cur, "[", "]");
        stream.bump(); // closing
        return true;
    }
    else if (skip_if_token(stream, "__attribute__"))
    {
        skip(stream, cur, "(");
        skip_bracket_count(stream, cur, "(", ")");
        skip(stream, cur, ")");
        return true;
    }

    return false;
}
