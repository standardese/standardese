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
            detail::skip(stream, cur, {"using", name.c_str(), "="});

            while (stream.peek().get_value() != ";")
            {
                detail::skip_attribute(stream, cur);
                target_name += stream.get().get_value().c_str();
            }
        }
        else
        {
            assert(clang_getCursorKind(cur) == CXCursor_TypedefDecl);

            skip(stream, cur, {"typedef"});

            while (stream.peek().get_value() != ";")
            {
                detail::skip_attribute(stream, cur);
                auto& val = stream.peek().get_value();
                if (val != name.c_str())
                    target_name += val.c_str();

                stream.bump();
            }
        }

        detail::erase_trailing_ws(target_name);
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

cpp_name cpp_type_alias::get_scope() const
{
    assert(has_parent());
    if (get_parent().get_entity_type() == cpp_entity::alias_template_t)
        // parent is an alias template, so it doesn't add a new scope
        return get_parent().get_scope();
    return cpp_entity::get_scope();
}
