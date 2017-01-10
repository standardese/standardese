// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_template.hpp>

#include <cassert>
#include <cctype>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/cpp_function.hpp>
#include <standardese/error.hpp>

using namespace standardese;

cpp_ptr<cpp_template_parameter> cpp_template_parameter::try_parse(translation_unit& tu,
                                                                  cpp_cursor        cur,
                                                                  const cpp_entity& parent)
{
    switch (clang_getCursorKind(cur))
    {
    case CXCursor_TemplateTypeParameter:
        return cpp_template_type_parameter::parse(tu, cur, parent);
    case CXCursor_NonTypeTemplateParameter:
        return cpp_non_type_template_parameter::parse(tu, cur, parent);
    case CXCursor_TemplateTemplateParameter:
        return cpp_template_template_parameter::parse(tu, cur, parent);
    default:
        break;
    }

    return nullptr;
}

cpp_name cpp_template_parameter::do_get_unique_name() const
{
    auto parent = get_semantic_parent();
    assert(parent);

    std::string name = get_name().c_str();
    if (name.empty())
    {
        auto i      = 0u;
        auto params = get_template_parameters(*parent);
        assert(params);
        for (auto& param : *params)
        {
            if (&param == this)
                break;
            else
                ++i;
        }

        name = std::to_string(i);
    }

    return std::string(".") + name;
}

cpp_ptr<cpp_template_type_parameter> cpp_template_type_parameter::parse(translation_unit& tu,
                                                                        cpp_cursor        cur,
                                                                        const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_TemplateTypeParameter);

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);
    auto              name   = detail::parse_name(cur);

    // skip typename
    auto keyword = cpp_template_parameter::cpp_typename;
    auto res     = detail::skip_if_token(stream, "typename");
    if (!res)
    {
        // it must be class
        detail::skip(stream, cur, "class");
        keyword = cpp_template_parameter::cpp_class;
    }

    // variadic parameter
    auto is_variadic = false;
    if (stream.peek().get_value() == "...")
    {
        stream.bump();
        is_variadic = true;
    }

    // skip name
    if (!name.empty())
        detail::skip(stream, cur, name.c_str());

    // default
    std::string def_name;
    if (stream.peek().get_value() == "=")
    {
        stream.bump();

        while (!stream.done())
            detail::append_token(def_name, stream.get().get_value());

        if (def_name.back() == '>' && tokenizer.need_unmunch())
            def_name.pop_back();
    }

    return detail::make_cpp_ptr<cpp_template_type_parameter>(cur, parent, keyword,
                                                             cpp_type_ref(def_name, CXType()),
                                                             is_variadic);
}

cpp_ptr<cpp_non_type_template_parameter> cpp_non_type_template_parameter::parse(
    translation_unit& tu, cpp_cursor cur, const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_NonTypeTemplateParameter);

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);
    auto              name   = detail::parse_name(cur);

    // given type
    std::string type_given;
    while (!stream.done() && stream.peek().get_value() != "..."
           && stream.peek().get_value() != name.c_str())
    {
        if (detail::skip_attribute(stream, cur))
            continue;
        detail::append_token(type_given, stream.get().get_value());
    }

    // variadic parameter
    auto is_variadic = false;
    if (stream.peek().get_value() == "...")
    {
        stream.bump();
        is_variadic = true;
    }

    // skip name
    if (!name.empty())
        detail::skip(stream, cur, name.c_str());

    // continue with type
    while (!stream.done() && stream.peek().get_value() != "=")
        detail::append_token(type_given, stream.get().get_value());

    // default
    std::string def;
    if (stream.peek().get_value() == "=")
    {
        stream.bump();

        while (!stream.done())
            detail::append_token(def, stream.get().get_value());

        if (tokenizer.need_unmunch())
        {
            assert(def.back() == '>');
            def.pop_back();
        }
    }

    auto type = clang_getCursorType(cur);
    return detail::make_cpp_ptr<cpp_non_type_template_parameter>(cur, parent,
                                                                 cpp_type_ref(std::move(type_given),
                                                                              type),
                                                                 std::move(def), is_variadic);
}

namespace
{
    struct template_template_info
    {
        cpp_template_parameter::cpp_keyword_kind keyword;
        bool                                     variadic;
    };

    template_template_info parse_template_template(translation_unit& tu, cpp_cursor cur,
                                                   const cpp_name& name)
    {
        detail::tokenizer tokenizer(tu, cur);
        auto              stream = detail::make_stream(tokenizer);

        auto keyword = cpp_template_parameter::cpp_typename;
        auto found   = false;
        while (!stream.done())
        {
            if (detail::skip_if_token(stream, "..."))
                found = true;
            else if (detail::skip_if_token(stream, name.c_str()))
                break;
            else if (detail::skip_if_token(stream, "typename"))
                keyword = cpp_template_parameter::cpp_typename;
            else if (detail::skip_if_token(stream, "class"))
                keyword = cpp_template_parameter::cpp_class;
            else if (!std::isspace(stream.get().get_value()[0]))
                found = false;
        }

        return {keyword, found};
    }
}

cpp_ptr<cpp_template_template_parameter> cpp_template_template_parameter::parse(
    translation_unit& tu, cpp_cursor cur, const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_TemplateTemplateParameter);

    auto name = detail::parse_name(cur);
    auto info = parse_template_template(tu, cur, name);
    auto result =
        detail::make_cpp_ptr<cpp_template_template_parameter>(cur, parent, info.keyword,
                                                              cpp_template_ref(), info.variadic);

    std::string def_name;
    detail::visit_children(cur, [&](CXCursor cur, CXCursor) {
        if (auto param = cpp_template_parameter::try_parse(tu, cur, *result))
            result->add_paramter(std::move(param));
        else if (clang_getCursorKind(cur) == CXCursor_TemplateRef)
            result->default_ = cpp_template_ref(cur, def_name + detail::parse_name(cur).c_str());
        else
        {
            assert(clang_getCursorKind(cur) == CXCursor_NamespaceRef);
            def_name += detail::parse_name(cur).c_str();
            def_name += "::";
            return CXChildVisit_Recurse;
        }

        return CXChildVisit_Continue;
    });

    return result;
}

bool standardese::is_full_specialization(translation_unit& tu, cpp_cursor cur)
{
    if (clang_getCursorKind(cur) != CXCursor_FunctionDecl
        && clang_getCursorKind(cur) != CXCursor_CXXMethod
        && clang_getCursorKind(cur) != CXCursor_Constructor
        && clang_getCursorKind(cur) != CXCursor_ClassDecl
        && clang_getCursorKind(cur) != CXCursor_StructDecl
        && clang_getCursorKind(cur) != CXCursor_UnionDecl)
        // not an entity that can be specialized
        return false;

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);

    return stream.get().get_value() == "template";
}

const cpp_function_base* standardese::get_function(const cpp_entity& e) STANDARDESE_NOEXCEPT
{
    if (is_function_like(e.get_entity_type()))
        return static_cast<const cpp_function_base*>(&e);
    else if (e.get_entity_type() == cpp_entity::function_template_t)
        return &static_cast<const cpp_function_template&>(e).get_function();
    else if (e.get_entity_type() == cpp_entity::function_template_specialization_t)
        return &static_cast<const cpp_function_template_specialization&>(e).get_function();
    return nullptr;
}

namespace
{
    // returns the end of the last parameter
    template <typename T>
    unsigned parse_parameters(translation_unit& tu, T& result, cpp_cursor cur)
    {
        cpp_cursor last;
        detail::visit_children(cur, [&](CXCursor cur, CXCursor) {
            if (auto ptr = cpp_template_parameter::try_parse(tu, cur, result))
            {
                last = cur;
                result.add_template_parameter(std::move(ptr));
                return CXChildVisit_Continue;
            }
            return CXChildVisit_Break;
        });

        unsigned unused, end;
        detail::get_range(tu, last, unused, end);
        return end;
    }

    // appends template paramters to name
    template <typename T>
    std::string get_template_name(std::string name, T& result)
    {
        name += "<";
        auto needs_comma = false;

        for (auto& param : result.get_template_parameters())
        {
            if (param.get_name().empty())
                continue;

            if (needs_comma)
                name += ", ";
            else
                needs_comma = true;

            name += param.get_name().c_str();
            if (param.is_variadic())
                name += "...";
        }
        name += ">";

        return name;
    }
}

cpp_ptr<cpp_function_template> cpp_function_template::parse(translation_unit& tu, cpp_cursor cur,
                                                            const cpp_entity& parent)
{
    auto result      = detail::make_cpp_ptr<cpp_function_template>(cur, parent);
    auto last_offset = parse_parameters(tu, *result, cur);

    auto func = cpp_function_base::try_parse(tu, cur, *result, last_offset);
    assert(func);
    result->func_ = std::move(func);
    return result;
}

cpp_name cpp_function_template::get_name() const
{
    return func_->get_name();
}

cpp_name cpp_function_template::get_signature() const
{
    return func_->get_signature();
}

cpp_name cpp_function_template::do_get_unique_name() const
{
    return std::string(cpp_entity::do_get_unique_name().c_str()) + get_signature().c_str();
}

cpp_function_template::cpp_function_template(cpp_cursor cur, const cpp_entity& parent)
: cpp_entity(get_entity_type(), cur, parent)
{
}

cpp_ptr<cpp_function_template_specialization> cpp_function_template_specialization::parse(
    translation_unit& tu, cpp_cursor cur, const cpp_entity& parent)
{
    auto result = detail::make_cpp_ptr<cpp_function_template_specialization>(cur, parent);

    auto func = cpp_function_base::try_parse(tu, cur, *result);
    assert(func);
    result->func_ = std::move(func);

    auto primary_cur = clang_getSpecializedCursorTemplate(cur);
    assert(!clang_Cursor_isNull(primary_cur));
    result->primary_ = cpp_template_ref(primary_cur, result->func_->get_name());
    return result;
}

cpp_name cpp_function_template_specialization::get_signature() const
{
    return func_->get_signature();
}

cpp_name cpp_function_template_specialization::do_get_unique_name() const
{
    return std::string(cpp_entity::do_get_unique_name().c_str()) + get_signature().c_str();
}

cpp_function_template_specialization::cpp_function_template_specialization(cpp_cursor        cur,
                                                                           const cpp_entity& parent)
: cpp_entity(get_entity_type(), cur, parent), name_("")
{
}

cpp_ptr<cpp_class_template> cpp_class_template::parse(translation_unit& tu, cpp_cursor cur,
                                                      const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_ClassTemplate);

    auto result = detail::make_cpp_ptr<cpp_class_template>(cur, parent);
    parse_parameters(tu, *result, cur);

    auto ptr = cpp_class::parse(tu, cur, *result);
    if (!ptr)
        return nullptr;
    result->class_ = std::move(ptr);
    return result;
}

cpp_name cpp_class_template::get_name() const
{
    return class_->get_name().c_str();
}

cpp_name cpp_class_template::do_get_unique_name() const
{
    return get_template_name(cpp_entity::do_get_unique_name().c_str(), *this);
}

cpp_ptr<cpp_class_template_full_specialization> cpp_class_template_full_specialization::parse(
    translation_unit& tu, cpp_cursor cur, const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_ClassDecl
           || clang_getCursorKind(cur) == CXCursor_StructDecl
           || clang_getCursorKind(cur) == CXCursor_UnionDecl);

    auto result = detail::make_cpp_ptr<cpp_class_template_full_specialization>(cur, parent);

    auto ptr = cpp_class::parse(tu, cur, *result);
    if (!ptr)
        return nullptr;
    result->class_ = std::move(ptr);

    auto primary_cur = clang_getSpecializedCursorTemplate(cur);
    assert(!clang_Cursor_isNull(primary_cur));
    result->primary_ = cpp_template_ref(primary_cur, result->class_->get_name());
    return result;
}

cpp_ptr<cpp_class_template_partial_specialization> cpp_class_template_partial_specialization::parse(
    translation_unit& tu, cpp_cursor cur, const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_ClassTemplatePartialSpecialization);

    auto result = detail::make_cpp_ptr<cpp_class_template_partial_specialization>(cur, parent);
    parse_parameters(tu, *result, cur);

    auto ptr = cpp_class::parse(tu, cur, *result);
    if (!ptr)
        return nullptr;
    result->class_ = std::move(ptr);

    auto primary_cur = clang_getSpecializedCursorTemplate(cur);
    assert(!clang_Cursor_isNull(primary_cur));
    result->primary_ = cpp_template_ref(primary_cur, result->class_->get_name());
    return result;
}

const cpp_class* standardese::get_class(const cpp_entity& e) STANDARDESE_NOEXCEPT
{
    if (e.get_entity_type() == cpp_entity::class_t)
        return static_cast<const cpp_class*>(&e);
    else if (e.get_entity_type() == cpp_entity::class_template_t)
        return &static_cast<const cpp_class_template&>(e).get_class();
    else if (e.get_entity_type() == cpp_entity::class_template_full_specialization_t)
        return &static_cast<const cpp_class_template_full_specialization&>(e).get_class();
    else if (e.get_entity_type() == cpp_entity::class_template_partial_specialization_t)
        return &static_cast<const cpp_class_template_partial_specialization&>(e).get_class();
    return nullptr;
}

#if CINDEX_VERSION_MINOR >= 32

cpp_ptr<cpp_alias_template> cpp_alias_template::parse(translation_unit& tu, cpp_cursor cur,
                                                      const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_TypeAliasTemplateDecl);

    auto result = detail::make_cpp_ptr<cpp_alias_template>(cur, parent);
    parse_parameters(tu, *result, cur);

    cpp_cursor type;
    detail::visit_children(cur, [&](cpp_cursor cur, cpp_cursor) {
        if (clang_getCursorKind(cur) == CXCursor_TypeAliasDecl)
        {
            type = cur;
            return CXChildVisit_Break;
        }
        return CXChildVisit_Continue;
    });

    result->type_ = cpp_type_alias::parse(tu, type, parent);
    assert(result->type_);
    return result;
}

#endif

cpp_name cpp_alias_template::get_name() const
{
    return type_->get_name();
}

cpp_name cpp_alias_template::do_get_unique_name() const
{
    return get_template_name(cpp_entity::do_get_unique_name().c_str(), *this);
}

const cpp_entity_container<cpp_template_parameter>* standardese::get_template_parameters(
    const cpp_entity& e)
{
    switch (e.get_entity_type())
    {
#define STANDARDESE_DETAIL_HANDLE(name)                                                            \
    case cpp_entity::name##_t:                                                                     \
        return &static_cast<const cpp_##name&>(e).get_template_parameters();

        STANDARDESE_DETAIL_HANDLE(function_template)

        STANDARDESE_DETAIL_HANDLE(class_template)
        STANDARDESE_DETAIL_HANDLE(class_template_partial_specialization)

        STANDARDESE_DETAIL_HANDLE(alias_template)

#undef STANDARDESE_DETAIL_HANDLE

    default:
        break;
    }

    return nullptr;
}
