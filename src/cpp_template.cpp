// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_template.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/search_token.hpp>
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

cpp_ptr<standardese::cpp_template_parameter> cpp_template_parameter::try_parse(const parser &p, cpp_cursor cur)
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

cpp_ptr<cpp_template_type_parameter> cpp_template_type_parameter::parse(const parser &, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_TemplateTypeParameter);

    bool is_variadic;
    auto def_name = detail::parse_template_type_default(cur, is_variadic);

    return detail::make_ptr<cpp_template_type_parameter>(detail::parse_name(cur), detail::parse_comment(cur),
                                                         cpp_type_ref({}, def_name), is_variadic);
}

cpp_ptr<cpp_non_type_template_parameter> cpp_non_type_template_parameter::parse(const parser &, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_NonTypeTemplateParameter);

    auto name = detail::parse_name(cur);
    bool is_variadic;
    std::string def;
    auto type_given = detail::parse_template_non_type_type(cur, name, def, is_variadic);

    auto type = clang_getCursorType(cur);

    return detail::make_ptr<cpp_non_type_template_parameter>(std::move(name), detail::parse_comment(cur),
                                                             cpp_type_ref(type, std::move(type_given)), std::move(def),
                                                             is_variadic);
}

namespace
{
    bool is_template_template_variadic(cpp_cursor cur, const cpp_name &name)
    {
        return detail::has_direct_prefix_token(cur, "...", name.c_str());
    }
}

cpp_ptr<cpp_template_template_parameter> cpp_template_template_parameter::parse(const parser &p, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_TemplateTemplateParameter);

    auto name = detail::parse_name(cur);
    auto variadic = is_template_template_variadic(cur, name);
    auto result = detail::make_ptr<cpp_template_template_parameter>(std::move(name), detail::parse_comment(cur),
                                                                    cpp_template_ref(), variadic);

    detail::visit_children(cur, [&](CXCursor cur, CXCursor)
    {
        if (auto param = cpp_template_parameter::try_parse(p, cur))
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
    void parse_parameters(const parser &p, T *result, cpp_cursor cur)
    {
        detail::visit_children(cur, [&](CXCursor cur, CXCursor)
        {
            if (auto ptr = cpp_template_parameter::try_parse(p, cur))
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
}

cpp_ptr<cpp_function_template> cpp_function_template::parse(const parser &p, cpp_name scope, cpp_cursor cur)
{
    auto func = cpp_function_base::try_parse(p, std::move(scope),
                                             cur);
    assert(func);

    auto result = detail::make_ptr<cpp_function_template>("", std::move(func));

    parse_parameters(p, result.get(), cur);
    result->set_name(get_template_name(result->func_->get_name(), result.get()));

    return result;
}

cpp_function_template::cpp_function_template(cpp_name template_name, cpp_ptr<cpp_function_base> ptr)
: cpp_entity(function_template_t, ptr->get_scope(), std::move(template_name), ptr->get_comment()),
  func_(std::move(ptr))
{}

cpp_ptr<cpp_function_template_specialization> cpp_function_template_specialization::parse(const parser &p,
                                                                                          cpp_name scope,
                                                                                          cpp_cursor cur)
{
    auto func = cpp_function_base::try_parse(p, std::move(scope), cur);
    assert(func);

    auto result = detail::make_ptr<cpp_function_template_specialization>("", std::move(func));

    result->set_name(detail::parse_template_specialization_name(cur, result->func_->get_name()));

    return result;
}

cpp_function_template_specialization::cpp_function_template_specialization(cpp_name template_name,
                                                                           cpp_ptr<cpp_function_base> ptr)
: cpp_entity(function_template_specialization_t, ptr->get_scope(), std::move(template_name), ptr->get_comment()),
  func_(std::move(ptr))
{}

cpp_class_template::parser::parser(const standardese::parser &p, cpp_name scope, cpp_cursor cur)
: parser_(p, scope, cur), class_(new cpp_class_template(std::move(scope), detail::parse_comment(cur)))
{
    assert(clang_getCursorKind(cur) == CXCursor_ClassTemplate);
    parse_parameters(p, class_.get(), cur);
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

bool standardese::is_full_specialization(cpp_cursor cur)
{
    bool result;
    detail::visit_tokens(cur, [&](CXToken, const string &spelling)
    {
        result = spelling == "template";
        return false;
    });

    return result;
}

cpp_class_template_full_specialization::parser::parser(const standardese::parser &p, cpp_name scope, cpp_cursor cur)
: parser_(p, scope, cur),
  class_(new cpp_class_template_full_specialization(std::move(scope), detail::parse_comment(cur)))
{
    assert(clang_getCursorKind(cur) == CXCursor_ClassDecl
        || clang_getCursorKind(cur) == CXCursor_StructDecl
        || clang_getCursorKind(cur) == CXCursor_UnionDecl);

    auto name = parser_.scope_name();
    class_->set_name(detail::parse_template_specialization_name(cur, name));
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

cpp_class_template_partial_specialization::parser::parser(const standardese::parser &p, cpp_name scope, cpp_cursor cur)
: parser_(p, scope, cur),
  class_(new cpp_class_template_partial_specialization(std::move(scope), detail::parse_comment(cur)))
{
    assert(clang_getCursorKind(cur) == CXCursor_ClassTemplatePartialSpecialization);
    parse_parameters(p, class_.get(), cur);

    auto name = parser_.scope_name();
    class_->set_name(detail::parse_template_specialization_name(cur, name));
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
