// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_variable.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

namespace
{
    cpp_type_ref parse_variable_type(translation_unit& tu, cpp_cursor cur, std::string& initializer,
                                     bool& is_thread_local, bool& is_mutable)
    {
        detail::tokenizer tokenizer(tu, cur);
        auto              stream = detail::make_stream(tokenizer);
        auto              name   = detail::parse_name(cur);

        std::string type_name;
        for (auto in_type = true, was_bitfield = false; stream.peek().get_value() != ";";)
        {
            detail::skip_attribute(stream, cur);

            if (detail::skip_if_token(stream, name.c_str())
                || detail::skip_if_token(stream, "extern")
                || detail::skip_if_token(stream, "static"))
                // ignore
                continue;
            else if (detail::skip_if_token(stream, "thread_local"))
                is_thread_local = true;
            else if (detail::skip_if_token(stream, "mutable"))
                is_mutable = true;
            else if (detail::skip_if_token(stream, ":"))
                was_bitfield = true;
            else if (was_bitfield)
            {
                stream.bump();
                was_bitfield = false;
                detail::skip_whitespace(stream);
            }
            else if (detail::skip_if_token(stream, "="))
                in_type = false;
            else
                (in_type ? type_name : initializer) += stream.get().get_value().c_str();
        }

        detail::erase_trailing_ws(type_name);
        detail::erase_trailing_ws(initializer);

        return {std::move(type_name), clang_getCursorType(cur)};
    }
}

cpp_ptr<cpp_variable> cpp_variable::parse(translation_unit& tu, cpp_cursor cur,
                                          const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_VarDecl);

    std::string initializer;
    auto        is_thread_local = false;
    auto        is_mutable      = false;
    auto        type = parse_variable_type(tu, cur, initializer, is_thread_local, is_mutable);
    if (is_mutable)
        throw parse_error(source_location(cur), "non-member variable is mutable");

    return detail::make_cpp_ptr<cpp_variable>(cur, parent, std::move(type), std::move(initializer),
                                              is_thread_local);
}

cpp_ptr<cpp_member_variable_base> cpp_member_variable_base::parse(translation_unit& tu,
                                                                  cpp_cursor        cur,
                                                                  const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_FieldDecl);

    std::string initializer;
    auto        is_thread_local = false;
    auto        is_mutable      = false;
    auto        type = parse_variable_type(tu, cur, initializer, is_thread_local, is_mutable);
    if (is_thread_local)
        throw parse_error(source_location(cur), "member variable is thread local");

    cpp_ptr<cpp_member_variable_base> result;
    if (clang_Cursor_isBitField(cur))
    {
        auto no_bits = clang_getFieldDeclBitWidth(cur);
        result       = detail::make_cpp_ptr<cpp_bitfield>(cur, parent, std::move(type),
                                                    std::move(initializer), no_bits, is_mutable);
    }
    else
        result = detail::make_cpp_ptr<cpp_member_variable>(cur, parent, std::move(type),
                                                           std::move(initializer), is_mutable);
    return result;
}
