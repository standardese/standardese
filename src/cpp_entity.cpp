// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_entity.hpp>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/cpp_class.hpp>
#include <standardese/cpp_enum.hpp>
#include <standardese/cpp_function.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/cpp_type.hpp>
#include <standardese/cpp_variable.hpp>
#include <standardese/error.hpp>
#include <standardese/translation_unit.hpp>

#include <spdlog/fmt/fmt.h>

using namespace standardese;

cpp_entity_ptr cpp_entity::try_parse(translation_unit& tu, cpp_cursor cur, const cpp_entity& parent)
{
    auto kind = clang_getCursorKind(cur);
    if (kind != CXCursor_CXXBaseSpecifier && !clang_isDeclaration(kind))
        return nullptr;

    switch (kind)
    {
#define STANDARDESE_DETAIL_HANDLE(Kind, Type)                                                      \
    case CXCursor_##Kind:                                                                          \
        return cpp_##Type::parse(tu, cur, parent);

#define STANDARDESE_DETAIL_HANDLE_TMP(Kind, TypeA, TypeB)                                          \
    case CXCursor_##Kind:                                                                          \
        if (is_full_specialization(tu, cur))                                                       \
            return cpp_##TypeA::parse(tu, cur, parent);                                            \
        else                                                                                       \
            return cpp_##TypeB::parse(tu, cur, parent);

        STANDARDESE_DETAIL_HANDLE(Namespace, namespace)
        STANDARDESE_DETAIL_HANDLE(NamespaceAlias, namespace_alias)
        STANDARDESE_DETAIL_HANDLE(UsingDirective, using_directive)
        STANDARDESE_DETAIL_HANDLE(UsingDeclaration, using_declaration)

    case CXCursor_TypedefDecl:
        STANDARDESE_DETAIL_HANDLE(TypeAliasDecl, type_alias)

        STANDARDESE_DETAIL_HANDLE(EnumDecl, enum)
        STANDARDESE_DETAIL_HANDLE(EnumConstantDecl, enum_value)

        STANDARDESE_DETAIL_HANDLE(VarDecl, variable)
        STANDARDESE_DETAIL_HANDLE(FieldDecl, member_variable_base)

        STANDARDESE_DETAIL_HANDLE_TMP(FunctionDecl, function_template_specialization, function)
        STANDARDESE_DETAIL_HANDLE_TMP(CXXMethod, function_template_specialization, member_function)
        STANDARDESE_DETAIL_HANDLE(ConversionFunction, conversion_op)
        STANDARDESE_DETAIL_HANDLE_TMP(Constructor, function_template_specialization, constructor)
        STANDARDESE_DETAIL_HANDLE(Destructor, destructor)

        STANDARDESE_DETAIL_HANDLE(FunctionTemplate, function_template)

    case CXCursor_StructDecl:
    case CXCursor_UnionDecl:
        STANDARDESE_DETAIL_HANDLE_TMP(ClassDecl, class_template_full_specialization, class)

        STANDARDESE_DETAIL_HANDLE(ClassTemplate, class_template)
        STANDARDESE_DETAIL_HANDLE(ClassTemplatePartialSpecialization,
                                  class_template_partial_specialization)

        STANDARDESE_DETAIL_HANDLE(CXXBaseSpecifier, base_class)
        STANDARDESE_DETAIL_HANDLE(CXXAccessSpecifier, access_specifier)

#if CINDEX_VERSION_MINOR >= 32
        STANDARDESE_DETAIL_HANDLE(TypeAliasTemplateDecl, alias_template)
#endif

#undef STANDARDESE_DETAIL_HANDLE
#undef STANDARDESE_DETAIL_HANDLE_TMP

    // ignored, because handled elsewhere
    case CXCursor_TemplateTypeParameter:
    case CXCursor_TemplateTemplateParameter:
    case CXCursor_NonTypeTemplateParameter:
    case CXCursor_ParmDecl:
        return nullptr;

// ignored, because not needed
#if CINDEX_VERSION_MINOR >= 35
    case CXCursor_StaticAssert:
        return nullptr;
#endif

    default:
        break;
    }

    detail::tokenizer tokenizer(tu, cur);
    // check for extern "C" specifier
    if (tokenizer.begin()->get_value() == "extern")
        return cpp_language_linkage::parse(tu, cur, parent);
    // check for friend
    else if (tokenizer.begin()->get_value() == "friend")
        // ignore it, the friend function definitions are transformed
        return nullptr;

    auto spelling = string(clang_getCursorKindSpelling(clang_getCursorKind(cur)));
    throw parse_error(source_location(cur),
                      fmt::format("Unknown cursor kind '{}'", spelling.c_str()), severity::warning);
}

cpp_name cpp_entity::get_name() const
{
    return detail::parse_name(cursor_);
}

namespace
{
    std::string get_scope_impl(const cpp_entity& e, bool unique_name)
    {
        auto parent = e.get_semantic_parent();
        if (!parent || parent->get_entity_type() == cpp_entity::file_t)
            return "";
        else if (parent->get_entity_type() == cpp_entity::language_linkage_t)
            return get_scope_impl(*parent, unique_name);

        auto name = unique_name ? parent->get_unique_name() : parent->get_full_name();
        // remove trailing ::, if any
        return std::string(name.c_str(), name.end()[-1] == ':' ? name.length() - 2 : name.length());
    }
}

cpp_name cpp_entity::get_scope() const
{
    return get_scope_impl(*this, false);
}

cpp_name cpp_entity::do_get_unique_name() const
{
    auto scope = get_scope_impl(*this, true);
    if (scope.empty())
        return get_name().c_str();
    return scope + "::" + get_name().c_str();
}

cpp_entity::cpp_entity(type t, cpp_cursor cur, const cpp_entity& parent)
: cursor_(cur), next_(nullptr), parent_(&parent), t_(t)
{
}

cpp_entity::cpp_entity(type t, cpp_cursor cur)
: cursor_(cur), next_(nullptr), parent_(nullptr), t_(t)
{
}