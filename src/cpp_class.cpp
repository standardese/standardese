// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_class.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>

using namespace standardese;

const char* standardese::to_string(cpp_access_specifier_t access) STANDARDESE_NOEXCEPT
{
    switch (access)
    {
        case cpp_private:
            return "private";
        case cpp_protected:
            return "protected";
        case cpp_public:
            return "public";
    }

    return "should never get here, Simba";
}

bool standardese::is_base_of(const cpp_class &base, const cpp_class &derived) STANDARDESE_NOEXCEPT
{
    if (base.get_name() == derived.get_name())
        // same non-union class
        return base.get_class_type() != cpp_union_t;
    else if (base.is_final())
        return false;

    for (auto& e : derived)
        if (e.get_entity_type() == cpp_entity::base_class_t
            && e.get_name() == base.get_name())
            return true;
        else if (e.get_entity_type() != cpp_entity::base_class_t)
            break;

    return false;
}

namespace
{
   cpp_access_specifier_t parse_access_specifier(cpp_cursor cur)
   {
       switch (clang_getCXXAccessSpecifier(cur))
       {
           case CX_CXXPrivate:
               return cpp_private;
           case CX_CXXProtected:
               return cpp_protected;
           case CX_CXXPublic:
               return cpp_public;
           default:
               break;
       }
       assert(false);
   }
}

cpp_ptr<cpp_access_specifier> cpp_access_specifier::parse(translation_unit &,
                                                          cpp_cursor cur, const cpp_entity &parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_CXXAccessSpecifier);

    return detail::make_ptr<cpp_access_specifier>(cur, parent,
                                                  parse_access_specifier(cur));
}

cpp_ptr<cpp_base_class> cpp_base_class::parse(translation_unit &,
                                              cpp_cursor cur, const cpp_entity &parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_CXXBaseSpecifier);

    auto name = detail::parse_class_name(cur);
    auto type = clang_getCursorType(cur);
    auto a = parse_access_specifier(cur);
    auto virt = clang_isVirtualBase(cur);

    return detail::make_ptr<cpp_base_class>(cur, parent, cpp_type_ref(std::move(name), type), a, virt);
}

cpp_name cpp_base_class::get_name() const
{
    return type_.get_name();
}

namespace
{
    cpp_class_type parse_class_type(cpp_cursor cur)
    {
        auto kind = clang_getCursorKind(cur);
        if (kind == CXCursor_ClassTemplate
            || kind == CXCursor_ClassTemplatePartialSpecialization)
            kind = clang_getTemplateCursorKind(cur);

        if (kind == CXCursor_ClassDecl)
            return cpp_class_t;
        else if (kind == CXCursor_StructDecl)
            return cpp_struct_t;
        else if (kind == CXCursor_UnionDecl)
            return cpp_union_t;

        assert(!"invalid cursor type for cpp_class");
        return cpp_class_t;
    }

    bool parse_class(translation_unit &tu, cpp_cursor cur, bool &is_final)
    {
        detail::tokenizer tokenizer(tu, cur);
        auto stream = detail::make_stream(tokenizer);
        auto name = detail::parse_name(cur);

        // handle extern templates
        if (detail::skip_if_token(stream, "extern"))
            return false;

        if (detail::skip_if_token(stream, "template"))
        {
            detail::skip_bracket_count(stream, cur, "<", ">");
            detail::skip_whitespace(stream);
        }

        // skip class/struct/union/keyword and name
        stream.bump();
        detail::skip_whitespace(stream);
        detail::skip_attribute(stream, cur);
        detail::skip_whitespace(stream);
        detail::skip(stream, cur, {name.c_str()});

        if (stream.peek().get_value() == "<")
        {
            detail::skip_bracket_count(stream, cur, "<", ">");
            detail::skip_whitespace(stream);
        }

        if (stream.peek().get_value() == "final")
        {
            stream.bump();
            detail::skip_whitespace(stream);
            is_final = true;
        }
        else
            is_final = false;

        return stream.peek().get_value() != ";";
    }
}

cpp_ptr<cpp_class> cpp_class::parse(translation_unit &tu, cpp_cursor cur, const cpp_entity &parent)
{
    auto ctype = parse_class_type(cur);

    auto is_final = false;
    auto definition = parse_class(tu, cur, is_final);
    if (!definition)
        return nullptr;

    return detail::make_ptr<cpp_class>(cur, parent, ctype, is_final);
}
