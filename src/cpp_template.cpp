// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_template.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/cpp_function.hpp>
#include <standardese/string.hpp>

using namespace standardese;

cpp_name cpp_template_ref::get_full_name() const
{
    string spelling(clang_getCursorSpelling(declaration_));
    assert(spelling.get() == given_);

    auto scope = detail::parse_scope(declaration_);
    return scope.empty() ? spelling.get() : scope + "::" + spelling.get();
}

cpp_ptr<standardese::cpp_template_parameter> cpp_template_parameter::try_parse(translation_unit &p, cpp_cursor cur)
{
    switch (clang_getCursorKind(cur))
    {
        case CXCursor_TemplateTypeParameter:
            return cpp_template_type_parameter::parse(p, cur);
        case CXCursor_NonTypeTemplateParameter:
            return cpp_non_type_template_parameter::parse(p, cur);
        case CXCursor_TemplateTemplateParameter:
            return cpp_template_template_parameter::parse(p, cur);
        default:
            break;
    }

    return nullptr;
}

cpp_ptr<cpp_template_type_parameter> cpp_template_type_parameter::parse(translation_unit &tu, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_TemplateTypeParameter);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    auto name = detail::parse_name(cur);

    // skip typename
    auto res = detail::skip_if_token(stream, "typename");
    if (!res)
        res = detail::skip_if_token(stream, "class");
    assert(res);

    // variadic parameter
    auto is_variadic = false;
    if (stream.peek().get_value() == "...")
    {
        stream.bump();
        detail::skip_whitespace(stream);
        is_variadic = true;
    }

    // skip name
    if (!name.empty())
        detail::skip(stream, {name.c_str()});

    // default
    cpp_name def_name;
    if (stream.peek().get_value() == "=")
    {
        stream.bump();
        detail::skip_whitespace(stream);

        while (!stream.done())
            def_name += stream.get().get_value().c_str();

        while (std::isspace(def_name.back()))
            def_name.pop_back();
        detail::unmunch(def_name);
    }

    return detail::make_ptr<cpp_template_type_parameter>(std::move(name), detail::parse_comment(cur),
                                                         cpp_type_ref({}, def_name), is_variadic);
}

cpp_ptr<cpp_non_type_template_parameter> cpp_non_type_template_parameter::parse(translation_unit &tu, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_NonTypeTemplateParameter);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    auto name = detail::parse_name(cur);

    // given type
    cpp_name type_given;
    while (stream.peek().get_value() != "..."
        && stream.peek().get_value() != name.c_str())
        type_given += stream.get().get_value().c_str();

    // variadic parameter
    auto is_variadic = false;
    if (stream.peek().get_value() == "...")
    {
        stream.bump();
        detail::skip_whitespace(stream);
        is_variadic = true;
    }

    // skip name
    if (!name.empty())
        detail::skip(stream, {name.c_str()});

    // continue with type
    while (!stream.done() && stream.peek().get_value() != "=")
        type_given += stream.get().get_value().c_str();

    while (std::isspace(type_given.back()))
        type_given.pop_back();
    detail::unmunch(type_given);

    // default
    cpp_name def;
    if (stream.peek().get_value() == "=")
    {
        stream.bump();
        detail::skip_whitespace(stream);

        while (!stream.done())
            def += stream.get().get_value().c_str();

        while (std::isspace(def.back()))
            def.pop_back();
        detail::unmunch(def);
    }

    auto type = clang_getCursorType(cur);

    return detail::make_ptr<cpp_non_type_template_parameter>(std::move(name), detail::parse_comment(cur),
                                                             cpp_type_ref(type, std::move(type_given)), std::move(def),
                                                             is_variadic);
}

namespace
{
    bool is_template_template_variadic(translation_unit &tu, cpp_cursor cur, const cpp_name &name)
    {
        detail::tokenizer tokenizer(tu, cur);
        auto stream = detail::make_stream(tokenizer);

        auto found = false;
        while (!stream.done())
        {
            if (detail::skip_if_token(stream, "..."))
                found = true;
            else if (detail::skip_if_token(stream, name.c_str()))
                break;
            else if (!std::isspace(stream.get().get_value()[0]))
                found = false;
        }

        return found;
    }
}

cpp_ptr<cpp_template_template_parameter> cpp_template_template_parameter::parse(translation_unit &tu, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_TemplateTemplateParameter);

    auto name = detail::parse_name(cur);
    auto variadic = is_template_template_variadic(tu, cur, name);
    auto result = detail::make_ptr<cpp_template_template_parameter>(std::move(name), detail::parse_comment(cur),
                                                                    cpp_template_ref(), variadic);

    detail::visit_children(cur, [&](CXCursor cur, CXCursor)
    {
        if (auto param = cpp_template_parameter::try_parse(tu, cur))
           result->add_paramter(std::move(param));
        else
        {
           assert(clang_getCursorKind(cur) == CXCursor_TemplateRef);
           string spelling(clang_getCursorSpelling(cur));
           result->default_ = cpp_template_ref(clang_getCursorReferenced(cur), spelling.get());
        }

        return CXChildVisit_Continue;
    });

    return result;
}

namespace
{
    template <typename T>
    void parse_parameters(translation_unit &tu, T *result, cpp_cursor cur)
    {
        detail::visit_children(cur, [&](CXCursor cur, CXCursor)
        {
            if (auto ptr = cpp_template_parameter::try_parse(tu, cur))
            {
                result->add_template_parameter(std::move(ptr));
                return CXChildVisit_Continue;
            }
            return CXChildVisit_Break;
        });
    }

    template <typename T>
    cpp_name get_template_name(cpp_name name, T *result)
    {
        name += "<";
        auto needs_comma = false;
        for (auto& param : result->get_template_parameters())
        {
            if (needs_comma)
                name += ", ";
            else
                needs_comma = true;

            name += param.get_name();
            if (param.is_variadic())
                name += "...";
        }
        name += ">";

        return name;
    }

    cpp_name get_template_specialization_name(translation_unit &tu, cpp_cursor cur, const cpp_name &name)
    {
        if (name.empty())
            return "";

        detail::tokenizer tokenizer(tu, cur);
        auto stream = detail::make_stream(tokenizer);

        while (stream.get().get_value() != name.c_str())
            ;

        auto result = name + "<";

        auto bracket_count = 1;
        detail::skip(stream, "<");
        while (bracket_count == 1)
        {
            auto spelling = stream.get().get_value();

            if (spelling == "<")
                ++bracket_count;
            else if (spelling == ">")
                --bracket_count;

            result += spelling.c_str();
        }

        return result;
    }
}

cpp_ptr<cpp_function_template> cpp_function_template::parse(translation_unit &tu, cpp_name scope, cpp_cursor cur)
{
    auto func = cpp_function_base::try_parse(tu, std::move(scope),
                                             cur);
    assert(func);

    auto result = detail::make_ptr<cpp_function_template>("", std::move(func));

    parse_parameters(tu, result.get(), cur);
    result->set_name(get_template_name(result->func_->get_name(), result.get()));

    return result;
}

cpp_function_template::cpp_function_template(cpp_name template_name, cpp_ptr<cpp_function_base> ptr)
: cpp_entity(function_template_t, ptr->get_scope(), std::move(template_name), ptr->get_comment()),
  func_(std::move(ptr))
{}

cpp_ptr<cpp_function_template_specialization> cpp_function_template_specialization::parse(translation_unit &tu,
                                                                                          cpp_name scope,
                                                                                          cpp_cursor cur)
{
    auto func = cpp_function_base::try_parse(tu, std::move(scope), cur);
    assert(func);

    auto result = detail::make_ptr<cpp_function_template_specialization>("", std::move(func));

    result->set_name(get_template_specialization_name(tu, cur, result->func_->get_name()));

    return result;
}

cpp_function_template_specialization::cpp_function_template_specialization(cpp_name template_name,
                                                                           cpp_ptr<cpp_function_base> ptr)
: cpp_entity(function_template_specialization_t, ptr->get_scope(), std::move(template_name), ptr->get_comment()),
  func_(std::move(ptr))
{}

cpp_class_template::parser::parser(translation_unit &tu, cpp_name scope, cpp_cursor cur)
: parser_(tu, scope, cur), class_(new cpp_class_template(std::move(scope), detail::parse_comment(cur)))
{
    assert(clang_getCursorKind(cur) == CXCursor_ClassTemplate);
    parse_parameters(tu, class_.get(), cur);
    class_->set_name(get_template_name(parser_.scope_name(), class_.get()));
}

cpp_entity_ptr cpp_class_template::parser::finish(const standardese::parser &par)
{
    auto ptr = static_cast<cpp_class*>(parser_.finish(par).release());
    if (!ptr)
        return nullptr;

    class_->class_ = cpp_ptr<cpp_class>(ptr);
    return std::move(class_);
}

cpp_class_template::cpp_class_template(cpp_name template_name, cpp_ptr<cpp_class> ptr)
: cpp_entity(class_template_t, ptr->get_scope(), std::move(template_name), ptr->get_comment()),
  class_(std::move(ptr))
{}

cpp_class_template::cpp_class_template(cpp_name scope, cpp_name comment)
: cpp_entity(class_template_t, std::move(scope), "", std::move(comment)), class_(nullptr) {}

bool standardese::is_full_specialization(translation_unit &tu, cpp_cursor cur)
{
    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    return stream.get().get_value() == "template";
}

cpp_class_template_full_specialization::parser::parser(translation_unit &tu, cpp_name scope, cpp_cursor cur)
: parser_(tu, scope, cur),
  class_(new cpp_class_template_full_specialization(std::move(scope), detail::parse_comment(cur)))
{
    assert(clang_getCursorKind(cur) == CXCursor_ClassDecl
        || clang_getCursorKind(cur) == CXCursor_StructDecl
        || clang_getCursorKind(cur) == CXCursor_UnionDecl);

    auto name = parser_.scope_name();
    class_->set_name(get_template_specialization_name(tu, cur, name));
    class_->template_ = cpp_template_ref(clang_getSpecializedCursorTemplate(cur), std::move(name));
}

cpp_entity_ptr cpp_class_template_full_specialization::parser::finish(const standardese::parser &par)
{
    auto ptr = static_cast<cpp_class*>(parser_.finish(par).release());
    if (!ptr)
        return nullptr;

    class_->class_ = cpp_ptr<cpp_class>(ptr);
    return std::move(class_);
}

cpp_class_template_full_specialization::cpp_class_template_full_specialization(cpp_name template_name,
                                                                               cpp_ptr<cpp_class> ptr,
                                                                               cpp_template_ref primary)
: cpp_entity(class_template_full_specialization_t, ptr->get_scope(), std::move(template_name), ptr->get_comment()),
  class_(std::move(ptr)), template_(std::move(primary)) {}

cpp_class_template_full_specialization::cpp_class_template_full_specialization(cpp_name scope, cpp_raw_comment comment)
: cpp_entity(class_template_full_specialization_t, std::move(scope), "", std::move(comment)), class_(nullptr) {}

cpp_class_template_partial_specialization::parser::parser(translation_unit &tu, cpp_name scope, cpp_cursor cur)
: parser_(tu, scope, cur),
  class_(new cpp_class_template_partial_specialization(std::move(scope), detail::parse_comment(cur)))
{
    assert(clang_getCursorKind(cur) == CXCursor_ClassTemplatePartialSpecialization);
    parse_parameters(tu, class_.get(), cur);

    auto name = parser_.scope_name();
    class_->set_name(get_template_specialization_name(tu, cur, name));
    class_->template_ = cpp_template_ref(clang_getSpecializedCursorTemplate(cur), std::move(name));
}

cpp_entity_ptr cpp_class_template_partial_specialization::parser::finish(const standardese::parser &par)
{
    auto ptr = static_cast<cpp_class*>(parser_.finish(par).release());
    if (!ptr)
        return nullptr;

    class_->class_ = cpp_ptr<cpp_class>(ptr);
    return std::move(class_);
}

cpp_class_template_partial_specialization::cpp_class_template_partial_specialization(cpp_name template_name,
                                                                               cpp_ptr<cpp_class> ptr,
                                                                               cpp_template_ref primary)
: cpp_entity(class_template_partial_specialization_t, ptr->get_scope(), std::move(template_name), ptr->get_comment()),
  class_(std::move(ptr)), template_(std::move(primary)) {}

cpp_class_template_partial_specialization::cpp_class_template_partial_specialization(cpp_name scope, cpp_raw_comment comment)
: cpp_entity(class_template_partial_specialization_t, std::move(scope), "", std::move(comment)), class_(nullptr) {}
