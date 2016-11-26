// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_type.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

CXType cpp_type::get_type() const STANDARDESE_NOEXCEPT
{
    return clang_getCursorType(get_cursor());
}

cpp_type_ref::cpp_type_ref(cpp_name name, CXType type) : name_(std::move(name)), type_(type)
{
}

cpp_cursor cpp_type_ref::get_declaration() const STANDARDESE_NOEXCEPT
{
    return clang_getTypeDeclaration(type_);
}

cpp_name cpp_type_ref::get_full_name() const STANDARDESE_NOEXCEPT
{
    std::string name = detail::parse_name(type_).c_str();
    // if in a partial template specialization
    // libclang uses a weird internal name when referring to template parameters
    // erase that name from the list
    if (name.find("-parameter-") != std::string::npos)
        detail::erase_template_args(name);

    return name;
}

namespace
{
    cpp_name parse_alias_target(translation_unit& tu, cpp_cursor cur)
    {
        std::string target_name;

        detail::tokenizer tokenizer(tu, cur);
        auto              stream = detail::make_stream(tokenizer);
        auto              name   = detail::parse_name(cur);

        if (clang_getCursorKind(cur) == CXCursor_TypeAliasDecl)
        {
            detail::skip(stream, cur, {"using", name.c_str()});
            detail::skip_attribute(stream, cur);
            detail::skip(stream, cur, "=");

            while (stream.peek().get_value() != ";")
            {
                if (detail::skip_attribute(stream, cur))
                    continue;
                detail::append_token(target_name, stream.get().get_value());
            }
        }
        else
        {
            assert(clang_getCursorKind(cur) == CXCursor_TypedefDecl);

            skip(stream, cur, "typedef");

            while (stream.peek().get_value() != ";")
            {
                if (detail::skip_attribute(stream, cur))
                    continue;
                auto val = stream.peek().get_value();
                if (val != name.c_str())
                    detail::append_token(target_name, val);

                stream.bump();
            }
        }

        return target_name;
    }
}

cpp_ptr<cpp_type_alias> cpp_type_alias::parse(translation_unit& tu, cpp_cursor cur,
                                              const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_TypedefDecl
           || clang_getCursorKind(cur) == CXCursor_TypeAliasDecl);

    auto name = parse_alias_target(tu, cur);
    auto type = clang_getTypedefDeclUnderlyingType(cur);

    return detail::make_cpp_ptr<cpp_type_alias>(cur, parent, cpp_type_ref(name, type));
}

bool cpp_type_alias::is_templated() const STANDARDESE_NOEXCEPT
{
    assert(has_ast_parent());
    return get_ast_parent().get_entity_type() == cpp_entity::alias_template_t;
}
