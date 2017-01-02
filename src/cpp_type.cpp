// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_type.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/translation_unit.hpp>
#include <clang-c/Index.h>

using namespace standardese;

CXType cpp_type::get_type() const STANDARDESE_NOEXCEPT
{
    return clang_getCursorType(get_cursor());
}

cpp_type_ref::cpp_type_ref(cpp_name name, CXType type) : name_(std::move(name)), type_(type)
{
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
    CXType get_pointee(CXType type)
    {
        if (type.kind == CXType_Pointer || type.kind == CXType_LValueReference
            || type.kind == CXType_RValueReference)
            return clang_getPointeeType(type);
        return type;
    }
}

CXType cpp_type_ref::get_underlying_cxtype() const STANDARDESE_NOEXCEPT
{
    auto result = get_pointee(type_);

    auto element = clang_getElementType(type_);
    if (element.kind != CXType_Invalid)
        result = element;

    return get_pointee(result);
}

cpp_cursor cpp_type_ref::get_declaration() const STANDARDESE_NOEXCEPT
{
    return standardese::get_declaration(get_underlying_cxtype());
}

cpp_cursor standardese::get_declaration(CXType t) STANDARDESE_NOEXCEPT
{
    auto decl = clang_getTypeDeclaration(t);
    // check if we have a specialized cursor
    // this also works for members of templates
    auto special_decl = clang_getSpecializedCursorTemplate(decl);
    if (!clang_Cursor_isNull(special_decl))
        return special_decl;
    else if (clang_getCursorKind(decl) == CXCursor_TypeAliasDecl
             || clang_getCursorKind(decl) == CXCursor_TypedefDecl)
    {
        // clang_getSpecializedCursorTemplate() does not work for typedefs in a class template
        // so workaround and return the specialization of a the parent template instead
        auto cur = clang_getCursorSemanticParent(decl);
        while (clang_getCursorKind(cur) != CXCursor_TranslationUnit && !clang_Cursor_isNull(cur))
        {
            auto special_cur = clang_getSpecializedCursorTemplate(cur);
            if (!clang_Cursor_isNull(special_cur))
                return special_cur;
            cur = clang_getCursorSemanticParent(cur);
        }
    }
    return decl;
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
