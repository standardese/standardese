// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_function.hpp>

#include <cassert>
#include <cctype>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/error.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

cpp_ptr<cpp_function_parameter> cpp_function_parameter::parse(translation_unit& tu, cpp_cursor cur,
                                                              const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_ParmDecl);

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);
    auto              name   = detail::parse_name(cur);

    std::string type_name, default_value;
    for (auto in_type = true, qualified_name = false; !stream.done();)
    {
        if (detail::skip_attribute(stream, cur))
            continue;
        else if (!qualified_name && detail::skip_if_token(stream, name.c_str()))
            continue;
        else if (detail::skip_if_token(stream, "="))
            in_type = false;
        else
        {
            qualified_name = stream.peek().get_value() == "::";
            detail::append_token((in_type ? type_name : default_value), stream.get().get_value());
        }
    }

    return detail::make_cpp_ptr<cpp_function_parameter>(cur, parent,
                                                        cpp_type_ref(std::move(type_name),
                                                                     clang_getCursorType(cur)),
                                                        std::move(default_value));
}

cpp_name cpp_function_parameter::do_get_unique_name() const
{
    auto parent = get_semantic_parent();
    assert(parent);
    return std::string(parent->get_unique_name().c_str()) + "." + get_name().c_str();
}

cpp_ptr<cpp_function_base> cpp_function_base::try_parse(translation_unit& p, cpp_cursor cur,
                                                        const cpp_entity& parent,
                                                        unsigned          template_offset)
{
    auto kind = clang_getCursorKind(cur);
    if (kind == CXCursor_FunctionTemplate)
        kind = clang_getTemplateCursorKind(cur);

    switch (kind)
    {
    case CXCursor_FunctionDecl:
        return cpp_function::parse(p, cur, parent, template_offset);
    case CXCursor_CXXMethod:
        return cpp_member_function::parse(p, cur, parent, template_offset);
    case CXCursor_ConversionFunction:
        return cpp_conversion_op::parse(p, cur, parent, template_offset);
    case CXCursor_Constructor:
        return cpp_constructor::parse(p, cur, parent, template_offset);
    case CXCursor_Destructor:
        return cpp_destructor::parse(p, cur, parent, template_offset);
    default:
        break;
    }

    return nullptr;
}

bool cpp_function_base::is_templated() const STANDARDESE_NOEXCEPT
{
    return is_function_template(get_ast_parent().get_entity_type());
}

void cpp_function_base::set_template_specialization_name(cpp_name name)
{
    assert(get_ast_parent().get_entity_type() == cpp_entity::function_template_specialization_t);
    auto& non_const      = const_cast<cpp_entity&>(get_ast_parent()); // save here
    auto& specialization = static_cast<cpp_function_template_specialization&>(non_const);
    specialization.name_ = std::string(detail::parse_name(get_cursor()).c_str()) + name.c_str();
}

cpp_name cpp_function_base::do_get_unique_name() const
{
    return std::string(get_full_name().c_str()) + get_signature().c_str();
}

namespace
{
    void skip_template_parameters(detail::token_stream& stream, unsigned last_offset)
    {
        if (stream.peek().get_value() == "template")
        {
            stream.bump();
            detail::skip_if_token(stream, "<");
            detail::skip_offset(stream, last_offset);
            detail::skip_if_token(stream, ">");
        }
    }

    std::string parse_member_function_prefix(detail::token_stream& stream, cpp_cursor cur,
                                             const cpp_name& name, cpp_function_info& finfo,
                                             cpp_member_function_info& minfo)
    {
        std::string return_type;
        auto        allow_auto = false; // whether or not auto is allowed in return type

        auto qualified_name = false;
        while (qualified_name || !detail::skip_if_token(stream, name.c_str()))
        {
            qualified_name = false;

            if (detail::skip_attribute(stream, cur) || detail::skip_if_token(stream, "extern")
                || detail::skip_if_token(stream, "inline"))
                // ignored
                continue;
            else if (detail::skip_if_token(stream, "static"))
                minfo.virtual_flag = cpp_virtual_static;
            else if (detail::skip_if_token(stream, "constexpr"))
                finfo.set_flag(cpp_constexpr_fnc);
            else if (detail::skip_if_token(stream, "virtual"))
                minfo.virtual_flag = cpp_virtual_new;
            else if (!allow_auto && detail::skip_if_token(stream, "auto"))
                // ignored
                continue;
            else if (detail::skip_if_token(stream, "operator"))
            {
                // we have an operator
                // they can have multiple tokens as part of the name
                // so need to skip until either template parameters or normal parameters
                const char* ptr = name.c_str() + std::strlen("operator");
                while (true)
                {
                    while (std::isspace(*ptr))
                        ++ptr;

                    auto& spelling = stream.peek().get_value();
                    if (!std::isspace(spelling[0]))
                    {
                        auto res = std::strncmp(ptr, spelling.c_str(), spelling.length());
                        if (res != 0)
                            break;
                        ptr += spelling.length();
                    }

                    stream.bump();
                }
                break;
            }
            else
            {
                auto spelling = stream.get().get_value();
                if (spelling == "decltype")
                    allow_auto = true; // decltype return, allow auto in return type
                else if (spelling == "::")
                    qualified_name = true; // next name is qualified, can't be function name

                detail::append_token(return_type, spelling); // part of return type
            }
        }

        return return_type;
    }

    void skip_template_arguments(detail::token_stream& stream, std::string& args,
                                 bool returns_function)
    {
        if (stream.peek().get_value() == "<")
        {
            // we need to skip all arguments of a template specialization
            // for that, go to the end and go backwards until we're there
            auto save = stream.get_iter();
            stream.reset(stream.get_end());

            auto was_opening_paren = false;
            for (auto paren_count = 0u; stream.get_iter() != save; stream.bump_back())
            {
                auto& str = stream.peek().get_value();

                if (paren_count == unsigned(returns_function) && was_opening_paren && str == ">")
                {
                    // there are only two occurrences where parenthesis are allowed
                    // a) parameters, b) noexcept
                    // we are currently in neither expression and found a '>'
                    // this must be the end of the template arguments

                    // set specialization args
                    auto end = stream.get_iter();
                    stream.reset(save);
                    while (stream.get_iter() != end)
                        detail::append_token(args, stream.get().get_value());
                    detail::append_token(args, stream.get().get_value());
                    break;
                }

                if (str == "(")
                {
                    --paren_count;
                    was_opening_paren = true;
                }
                else if (str == ")")
                {
                    was_opening_paren = false;
                    ++paren_count;
                }
            }
        }
    }

    void skip_parameters(detail::token_stream& stream, cpp_cursor cur, bool& variadic)
    {
        variadic = false;

        // whether or not a variadic parameter can come
        // i.e. after first bracket or comma
        auto variadic_param = true;
        skip_bracket_count(stream, cur, "(", ")", [&](const char* spelling) {
            if (variadic_param && std::strcmp(spelling, "...") == 0)
                variadic = true;
            else if (!variadic_param && *spelling == ',')
                variadic_param = true;
            else if (!std::isspace(*spelling))
                variadic_param = false;
        });
    }

    std::string parse_noexcept(detail::token_stream& stream)
    {
        std::string expression;
        if (stream.peek().get_value() == "(")
        {
            // noexcept with arguments
            stream.bump();

            auto bracket_count = 1;
            while (bracket_count != 0)
            {
                auto spelling = stream.get().get_value();
                if (spelling == "(")
                    ++bracket_count;
                else if (spelling == ")")
                    --bracket_count;

                if (bracket_count != 0) // only when not last closing bracket
                    detail::append_token(expression, spelling);
            }
        }
        else
        {
            // noexcept without arguments
            expression = "true";
        }

        return expression;
    }

    // whether or not the stream refers to the end of a declaration
    // doesn't consume the terminating character
    bool is_declaration_end(detail::token_stream& stream, cpp_cursor cur,
                            bool& is_special_definition)
    {
        detail::skip_attribute(stream, cur);

        if (stream.peek().get_value().length() > 1u)
            return false;

        auto c = stream.peek().get_value()[0];
        if (c == ':' || c == ';' || c == '{')
        {
            is_special_definition = false;
            return true;
        }
        else if (c == '=')
        {
            is_special_definition = true;
            return true;
        }

        return false;
    }

    cpp_function_definition parse_special_definition(detail::token_stream& stream, cpp_cursor cur)
    {
        auto spelling = stream.get().get_value();

        if (spelling == "default")
            // defaulted function
            return cpp_function_definition_defaulted;
        else if (spelling == "delete")
            // deleted function
            return cpp_function_definition_deleted;
        else if (spelling == "0")
            // pure virtual function
            return cpp_function_definition_pure;

        throw parse_error(source_location(cur), std::string("unknown function definition \'= ")
                                                    + spelling.c_str() + "\'");
    }

    cpp_function_definition parse_definition(detail::token_stream& stream, cpp_cursor cur,
                                             bool special_definition)
    {
        if (special_definition)
        {
            detail::skip(stream, cur, "=");
            return parse_special_definition(stream, cur);
        }
        else if (stream.peek().get_value() == "{" || stream.peek().get_value() == ":")
        {
            stream.bump();
            return cpp_function_definition_normal;
        }
        else
        {
            detail::skip(stream, cur, ";");
            return cpp_function_declaration;
        }
    }

    std::string parse_member_function_suffix(detail::token_stream& stream, cpp_cursor cur,
                                             cpp_function_info&        finfo,
                                             cpp_member_function_info& minfo)
    {
        std::string trailing_return_type;

        auto special_definition = false;
        while (!is_declaration_end(stream, cur, special_definition))
        {
            assert(!stream.done());

            if (detail::skip_attribute(stream, cur))
                continue;
            else if (detail::skip_if_token(stream, ")"))
            {
                // return type was a function pointer
                // now come the arguments
                trailing_return_type += ")(";

                skip_bracket_count(stream, cur, "(", ")", [&](const char* str) {
                    detail::append_token(trailing_return_type, str);
                });
            }
            else if (detail::skip_if_token(stream, "->"))
            {
                // trailing return type
                while (!is_declaration_end(stream, cur, special_definition))
                {
                    auto spelling = stream.get().get_value();
                    detail::append_token(trailing_return_type, spelling);
                    if (spelling == "decltype")
                    {
                        detail::append_token(trailing_return_type, "(");
                        skip_bracket_count(stream, cur, "(", ")", [&](const char* str) {
                            detail::append_token(trailing_return_type, str);
                        });
                    }
                }
                break;
            }
            else if (detail::skip_if_token(stream, "const"))
                minfo.set_cv(cpp_cv_const);
            else if (detail::skip_if_token(stream, "volatile"))
                minfo.set_cv(cpp_cv_volatile);
            else if (detail::skip_if_token(stream, "&"))
                minfo.ref_qualifier = cpp_ref_lvalue;
            else if (detail::skip_if_token(stream, "&&"))
                minfo.ref_qualifier = cpp_ref_rvalue;
            else if (detail::skip_if_token(stream, "final"))
                minfo.virtual_flag = cpp_virtual_final;
            else if (detail::skip_if_token(stream, "override"))
                minfo.virtual_flag = cpp_virtual_overriden;
            else if (detail::skip_if_token(stream, "noexcept"))
            {
                finfo.explicit_noexcept   = true;
                finfo.noexcept_expression = parse_noexcept(stream);
            }
            else if (!std::isspace(stream.peek().get_value()[0]))
            {
                auto str = stream.get().get_value();
                throw parse_error(source_location(cur),
                                  "unexpected token \'" + std::string(str.c_str()) + "\'");
            }
            else
                // is whitespace, so consume
                stream.get();
        }

        finfo.definition = parse_definition(stream, cur, special_definition);
        if (finfo.definition == cpp_function_definition_pure)
            minfo.virtual_flag = cpp_virtual_pure;

        return trailing_return_type;
    }

    bool is_function_ptr(const std::string& return_type)
    {
        if (return_type.empty())
            return false;
        auto iter = return_type.rbegin();
        while (std::isspace(*iter))
            ++iter;
        return *iter == '*';
    }

    cpp_type_ref parse_member_function(detail::token_stream& stream, cpp_cursor cur,
                                       unsigned template_offset, const cpp_name& name,
                                       std::string& template_args, cpp_function_info& finfo,
                                       cpp_member_function_info& minfo)
    {
        skip_template_parameters(stream, template_offset);
        auto return_type = parse_member_function_prefix(stream, cur, name, finfo, minfo);

        skip_template_arguments(stream, template_args, is_function_ptr(return_type));

        // handle parameters
        auto variadic = false;
        skip_parameters(stream, cur, variadic);
        if (variadic)
            finfo.set_flag(cpp_variadic_fnc);

        detail::append_token(return_type, parse_member_function_suffix(stream, cur, finfo, minfo));
        if (return_type.empty())
            // we have a deduced return type
            return_type = "auto";

        if (finfo.noexcept_expression.empty())
            finfo.noexcept_expression = "false";

        return {std::move(return_type), clang_getCursorResultType(cur)};
    }

    void parse_parameters(translation_unit& tu, cpp_function_base* base, cpp_cursor cur)
    {
        // we cannot use clang_Cursor_getNumArguments(),
        // doesn't work for templates
        // luckily parameters are exposed as children nodes
        // but if returning a function pointer, its parameters are as well
        // so obtain number of return parameters
        // and ignore those

        auto type             = clang_getPointeeType(clang_getCursorResultType(cur));
        auto no_params_return = clang_getNumArgTypes(type);
        if (no_params_return == -1)
            no_params_return = 0;

        auto i = 0;
        detail::visit_children(cur, [&](CXCursor cur, CXCursor) {
            if (clang_getCursorKind(cur) == CXCursor_ParmDecl && i++ >= no_params_return)
                base->add_parameter(cpp_function_parameter::parse(tu, cur, *base));
            return CXChildVisit_Continue;
        });
    }

    std::string calc_signature(const cpp_entity_container<cpp_function_parameter>& parameters,
                               bool                                                variadic)
    {
        std::string result = "(";
        for (auto& param : parameters)
        {
            result += param.get_type().get_full_name().c_str();
            result += ',';
        }
        if (result.back() == ',')
            result.pop_back();

        if (variadic)
            result += ",...";

        result += ")";
        return result;
    }

    void member_signature(std::string& signature, const cpp_member_function_info& info)
    {
        if (is_const(info.cv_qualifier))
            signature += " const";
        if (is_volatile(info.cv_qualifier))
            signature += " volatile";

        if (info.ref_qualifier == cpp_ref_rvalue)
            signature += " &&";
        else if (info.ref_qualifier == cpp_ref_lvalue)
            signature += " &";
    }
}

cpp_ptr<cpp_function> cpp_function::parse(translation_unit& tu, cpp_cursor cur,
                                          const cpp_entity& parent, unsigned template_offset)
{
    assert(clang_getCursorKind(cur) == CXCursor_FunctionDecl
           || clang_getTemplateCursorKind(cur) == CXCursor_FunctionDecl);

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);
    auto              name   = detail::parse_name(cur);

    cpp_function_info        finfo;
    cpp_member_function_info minfo;
    std::string              template_args;
    auto                     return_type =
        parse_member_function(stream, cur, template_offset, name, template_args, finfo, minfo);
    if (is_virtual(minfo.virtual_flag))
        throw parse_error(source_location(cur), "virtual specifier on normal function");
    if (minfo.cv_qualifier != cpp_cv_none)
        throw parse_error(source_location(cur), "cv qualifier on normal function");
    if (minfo.ref_qualifier != cpp_ref_none)
        throw parse_error(source_location(cur), "ref qualifier on normal function");

    auto result =
        detail::make_cpp_ptr<cpp_function>(cur, parent, std::move(return_type), std::move(finfo));
    parse_parameters(tu, result.get(), cur);
    result->signature_ = calc_signature(result->get_parameters(), result->is_variadic());

    if (!template_args.empty())
        result->set_template_specialization_name(std::move(template_args));
    return result;
}

cpp_function::cpp_function(cpp_cursor cur, const cpp_entity& parent, cpp_type_ref ret,
                           cpp_function_info info)
: cpp_function_base(get_entity_type(), cur, parent, std::move(info)), return_(std::move(ret))
{
}

namespace
{
    bool is_implicit_virtual(cpp_cursor cur)
    {
        CXCursor* ptr  = nullptr;
        auto      size = 0u;
        clang_getOverriddenCursors(cur, &ptr, &size);

        if (ptr)
            clang_disposeOverriddenCursors(ptr);

        return ptr != nullptr;
    }
}

cpp_ptr<cpp_member_function> cpp_member_function::parse(translation_unit& tu, cpp_cursor cur,
                                                        const cpp_entity& parent,
                                                        unsigned          template_offset)
{
    assert(clang_getCursorKind(cur) == CXCursor_CXXMethod
           || clang_getTemplateCursorKind(cur) == CXCursor_CXXMethod);

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);
    auto              name   = detail::parse_name(cur);

    cpp_function_info        finfo;
    cpp_member_function_info minfo;
    std::string              template_args;
    auto                     return_type =
        parse_member_function(stream, cur, template_offset, name, template_args, finfo, minfo);

    auto result = detail::make_cpp_ptr<cpp_member_function>(cur, parent, std::move(return_type),
                                                            std::move(finfo), std::move(minfo));
    parse_parameters(tu, result.get(), cur);
    result->signature_ = calc_signature(result->get_parameters(), result->is_variadic());
    member_signature(result->signature_, result->info_);

    if ((result->get_virtual() == cpp_virtual_none || result->get_virtual() == cpp_virtual_new)
        && is_implicit_virtual(cur))
        // check for implicit virtual
        result->info_.virtual_flag = cpp_virtual_overriden;

    if (!template_args.empty())
        result->set_template_specialization_name(std::move(template_args));
    return result;
}

cpp_member_function::cpp_member_function(cpp_cursor cur, const cpp_entity& parent, cpp_type_ref ret,
                                         cpp_function_info finfo, cpp_member_function_info minfo)
: cpp_function_base(get_entity_type(), cur, parent, std::move(finfo)),
  return_(std::move(ret)),
  info_(std::move(minfo))
{
}

cpp_ptr<cpp_conversion_op> cpp_conversion_op::parse(translation_unit& tu, cpp_cursor cur,
                                                    const cpp_entity& parent,
                                                    unsigned          template_offset)
{
    assert(clang_getCursorKind(cur) == CXCursor_ConversionFunction
           || clang_getTemplateCursorKind(cur) == CXCursor_ConversionFunction);

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);

    skip_template_parameters(stream, template_offset);

    cpp_function_info        finfo;
    cpp_member_function_info minfo;

    // handle prefix
    while (!detail::skip_if_token(stream, "operator"))
    {
        if (detail::skip_attribute(stream, cur) || detail::skip_if_token(stream, "inline"))
            continue;
        else if (detail::skip_if_token(stream, "explicit"))
            finfo.set_flag(cpp_explicit_conversion);
        else if (detail::skip_if_token(stream, "constexpr"))
            finfo.set_flag(cpp_constexpr_fnc);
        else if (detail::skip_if_token(stream, "virtual"))
            minfo.virtual_flag = cpp_virtual_new;
        else if (!std::isspace(stream.peek().get_value()[0]))
        {
            auto str = stream.get().get_value();
            throw parse_error(source_location(cur),
                              "unexpected token \'" + std::string(str.c_str()) + "\'");
        }
        else
            // is whitespace, so consume
            stream.get();
    }

    std::string type;
    // skip until parameters
    while (stream.peek().get_value() != "(" && stream.peek().get_value() != "<")
        detail::append_token(type, stream.get().get_value());

    std::string type_template_args;
    skip_template_arguments(stream, type_template_args, false);
    // template_args belong to the type name
    type += std::move(type_template_args);
    // handle the rest until parameters
    while (stream.peek().get_value() != "(")
        detail::append_token(type, stream.get().get_value());

    auto variadic = false;
    skip_parameters(stream, cur, variadic);
    if (variadic)
        throw parse_error(source_location(cur), "conversion op is variadic");

    auto trailing_return_type = parse_member_function_suffix(stream, cur, finfo, minfo);
    if (!trailing_return_type.empty())
        throw parse_error(source_location(cur), "conversion op has trailing return type");

    if (finfo.noexcept_expression.empty())
        finfo.noexcept_expression = "false";

    auto result =
        detail::make_cpp_ptr<cpp_conversion_op>(cur, parent,
                                                cpp_type_ref(std::move(type),
                                                             clang_getCursorResultType(cur)),
                                                std::move(finfo), std::move(minfo));

    if ((result->get_virtual() == cpp_virtual_none || result->get_virtual() == cpp_virtual_new)
        && is_implicit_virtual(cur))
        // check for implicit virtual
        result->info_.virtual_flag = cpp_virtual_overriden;

    return result;
}

cpp_name cpp_conversion_op::get_name() const
{
    return std::string("operator ") + target_type_.get_name().c_str();
}

cpp_name cpp_conversion_op::get_signature() const
{
    std::string result = "()";
    member_signature(result, info_);
    return result;
}

cpp_ptr<cpp_constructor> cpp_constructor::parse(translation_unit& tu, cpp_cursor cur,
                                                const cpp_entity& parent, unsigned template_offset)
{
    assert(clang_getCursorKind(cur) == CXCursor_Constructor
           || clang_getTemplateCursorKind(cur) == CXCursor_Constructor);

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);
    skip_template_parameters(stream, template_offset);

    std::string name = detail::parse_name(cur).c_str();
    detail::erase_template_args(name);

    // handle prefix
    cpp_function_info info;
    while (!detail::skip_if_token(stream, name.c_str()))
    {
        if (detail::skip_attribute(stream, cur) || detail::skip_if_token(stream, "inline"))
            continue;
        else if (detail::skip_if_token(stream, "explicit"))
            info.set_flag(cpp_explicit_conversion);
        else if (detail::skip_if_token(stream, "constexpr"))
            info.set_flag(cpp_constexpr_fnc);
        else if (!std::isspace(stream.peek().get_value()[0]))
        {
            auto str = stream.get().get_value();
            throw parse_error(source_location(cur),
                              "unexpected token \'" + std::string(str.c_str()) + "\'");
        }
        else
            // is whitespace, so consume
            stream.get();
    }

    std::string template_args;
    skip_template_arguments(stream, template_args, false);

    // handle parameters
    auto variadic = false;
    skip_parameters(stream, cur, variadic);
    if (variadic)
        info.set_flag(cpp_variadic_fnc);

    // parse suffix
    auto special_definition = false;
    while (!is_declaration_end(stream, cur, special_definition))
    {
        assert(!stream.done());
        detail::skip_attribute(stream, cur);

        if (detail::skip_if_token(stream, "noexcept"))
        {
            info.explicit_noexcept   = true;
            info.noexcept_expression = parse_noexcept(stream);
        }
        else if (!std::isspace(stream.peek().get_value()[0]))
        {
            auto str = stream.get().get_value();
            throw parse_error(source_location(cur),
                              "unexpected token \'" + std::string(str.c_str()) + "\'");
        }
        else
            // is whitespace, so consume
            stream.get();
    }

    // parse definition
    info.definition = parse_definition(stream, cur, special_definition);
    if (info.definition == cpp_function_definition_pure)
        throw parse_error(source_location(cur), "constructor is pure virtual");

    if (!info.explicit_noexcept)
        info.noexcept_expression = "false";

    auto result = detail::make_cpp_ptr<cpp_constructor>(cur, parent, std::move(info));
    parse_parameters(tu, result.get(), cur);
    result->signature_ = calc_signature(result->get_parameters(), result->is_variadic());

    if (!template_args.empty())
        result->set_template_specialization_name(std::move(template_args));
    return result;
}

cpp_name cpp_constructor::get_name() const
{
    std::string str = cpp_entity::get_name().c_str();
    detail::erase_template_args(str);
    return str;
}

cpp_constructor::cpp_constructor(cpp_cursor cur, const cpp_entity& parent, cpp_function_info info)
: cpp_function_base(get_entity_type(), cur, parent, std::move(info))
{
}

cpp_ptr<cpp_destructor> cpp_destructor::parse(translation_unit& tu, cpp_cursor cur,
                                              const cpp_entity& parent, unsigned template_offset)
{
    assert(clang_getCursorKind(cur) == CXCursor_Destructor
           || clang_getTemplateCursorKind(cur) == CXCursor_Destructor);
    assert(template_offset == 0u);

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);

    std::string name = detail::parse_name(cur).c_str();
    detail::erase_template_args(name);

    cpp_function_info info;
    auto              virtual_flag = cpp_virtual_none;
    while (!detail::skip_if_token(stream, "~"))
    {
        if (detail::skip_attribute(stream, cur) || detail::skip_if_token(stream, "inline"))
            continue; // ignored
        else if (detail::skip_if_token(stream, "virtual"))
            virtual_flag = cpp_virtual_new;
        else if (detail::skip_if_token(stream, "constexpr"))
            info.set_flag(cpp_constexpr_fnc);
    }

    // skip name and arguments
    detail::skip(stream, cur, {&name[1], "(", ")"});

    // parse suffix
    auto special_definition = false;
    while (!is_declaration_end(stream, cur, special_definition))
    {
        assert(!stream.done());

        if (detail::skip_attribute(stream, cur))
            continue;
        if (detail::skip_if_token(stream, "final"))
            virtual_flag = cpp_virtual_final;
        else if (detail::skip_if_token(stream, "override"))
            virtual_flag = cpp_virtual_overriden;
        else if (detail::skip_if_token(stream, "noexcept"))
        {
            info.explicit_noexcept   = true;
            info.noexcept_expression = parse_noexcept(stream);
        }
        else if (!std::isspace(stream.peek().get_value()[0]))
        {
            auto str = stream.get().get_value();
            throw parse_error(source_location(cur),
                              "unexpected token \'" + std::string(str.c_str()) + "\'");
        }
        else
            // is whitespace, so consume
            stream.get();
    }

    // parse definition
    info.definition = parse_definition(stream, cur, special_definition);
    if (info.definition == cpp_function_definition_pure)
        virtual_flag = cpp_virtual_pure;

    // dtors are implictly noexcept
    if (!info.explicit_noexcept)
        info.noexcept_expression = "true";

    auto result = detail::make_cpp_ptr<cpp_destructor>(cur, parent, std::move(info), virtual_flag);
    if ((result->get_virtual() == cpp_virtual_none || result->get_virtual() == cpp_virtual_new)
        && is_implicit_virtual(cur))
        // check for implicit virtual
        result->virtual_ = cpp_virtual_overriden;
    return result;
}

cpp_name cpp_destructor::get_name() const
{
    std::string str = cpp_entity::get_name().c_str();
    detail::erase_template_args(str);
    return str;
}
