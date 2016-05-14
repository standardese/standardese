// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_function.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>

using namespace standardese;

namespace
{
    cpp_type_ref parse_parameter_type(translation_unit &tu, cpp_cursor cur,
                                      const cpp_name &name, std::string &default_value)
    {
        assert(clang_getCursorKind(cur) == CXCursor_ParmDecl);

        auto type = clang_getCursorType(cur);
        cpp_name type_name;

        detail::tokenizer tokenizer(tu, cur);
        auto stream = detail::make_stream(tokenizer);

        for (auto in_type = true; stream.peek().get_value() != ";";)
        {
            if (detail::skip_if_token(stream, name.c_str()))
                continue;
            else if (detail::skip_if_token(stream, "="))
                in_type = false;
            else
                (in_type ? type_name : default_value) += stream.get().get_value().c_str();
        }

        while (std::isspace(type_name.back()))
            type_name.pop_back();

        return {type, std::move(type_name)};
    }
}

cpp_ptr<cpp_function_parameter> cpp_function_parameter::parse(translation_unit &tu, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_ParmDecl);

    auto name = detail::parse_name(cur);
    std::string default_value;
    auto type = parse_parameter_type(tu, cur, name, default_value);

    return detail::make_ptr<cpp_function_parameter>(std::move(name), detail::parse_comment(cur),
                                                    std::move(type), std::move(default_value));
}

cpp_ptr<standardese::cpp_function_base> cpp_function_base::try_parse(translation_unit &p, cpp_name scope,
                                                                    cpp_cursor cur)
{
    auto kind = clang_getCursorKind(cur);
    if (kind == CXCursor_FunctionTemplate)
        kind = clang_getTemplateCursorKind(cur);

    switch (kind)
    {
        case CXCursor_FunctionDecl:
            return cpp_function::parse(p, std::move(scope), cur);
        case CXCursor_CXXMethod:
            return cpp_member_function::parse(p, std::move(scope), cur);
        case CXCursor_ConversionFunction:
            return cpp_conversion_op::parse(p, std::move(scope), cur);
        case CXCursor_Constructor:
            return cpp_constructor::parse(p, std::move(scope), cur);
        case CXCursor_Destructor:
            return cpp_destructor::parse(p, std::move(scope), cur);
        default:
            break;
    }

    return nullptr;
}

namespace
{
    void skip_template_parameter_declaration(detail::token_stream &stream)
    {
        if (stream.peek().get_value() == "template")
        {
            stream.bump();
            skip_bracket_count(stream, "<", ">");
            detail::skip_whitespace(stream);
        }
    }

    cpp_name parse_member_function_prefix(detail::token_stream &stream, const cpp_name &name,
                                          cpp_function_info &finfo, cpp_member_function_info &minfo)
    {
        cpp_name return_type;
        auto allow_auto = false; // whether or not auto is allowed in return type

        while (!detail::skip_if_token(stream, name.c_str()))
        {
            if (detail::skip_if_token(stream, "extern"))
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
                while (stream.peek().get_value() != "("
                       && stream.peek().get_value() != "<")
                    stream.bump();
                break;
            }
            else
            {
                auto spelling = stream.get().get_value();
                if (spelling == "decltype")
                    allow_auto = true; // decltype return, allow auto in return type
                return_type += spelling.c_str(); // part of return type
            }
        }

        return return_type;
    }

    void skip_template_arguments(detail::token_stream &stream)
    {
        if (stream.peek().get_value() == "<")
            skip_bracket_count(stream, "<", ">");
    }

    void skip_parameters(detail::token_stream &stream, bool &variadic)
    {
        variadic = false;

        // whether or not a variadic parameter can come
        // i.e. after first bracket or comma
        auto variadic_param = true;
        skip_bracket_count(stream, "(", ")",
                           [&](const char *spelling)
                           {
                               if (variadic_param && std::strcmp(spelling, "...") == 0)
                                   variadic = true;
                               else if (!variadic_param && *spelling == ',')
                                   variadic_param = true;
                               else if (!std::isspace(*spelling))
                                   variadic_param = false;
                           });
    }

    std::string parse_noexcept(detail::token_stream &stream)
    {
        detail::skip_whitespace(stream);

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
                    expression += spelling.c_str();
            }
        }
        else
        {
            // noexcept without arguments
            expression = "true";
        }

        return expression;
    }

    bool is_declaration_end(detail::token_stream &stream, bool &is_special_definition)
    {
        auto c = stream.peek().get_value()[0];
        if (c == ':' || c == ';' || c == '{')
        {
            stream.bump();
            detail::skip_whitespace(stream);
            is_special_definition = false;
            return true;
        }
        else if (c == '=')
        {
            stream.bump();
            detail::skip_whitespace(stream);
            is_special_definition = true;
            return true;
        }

        return false;
    }

    // return cpp_function_definition_normal for pure virtual
    // yes, this is hacky
    cpp_function_definition parse_special_definition(detail::token_stream &stream)
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
            return cpp_function_definition_normal;

        assert(false);
        return cpp_function_definition_normal;
    }

    cpp_name parse_member_function_suffix(detail::token_stream &stream,
                                    cpp_function_info &finfo, cpp_member_function_info &minfo)
    {
        cpp_name trailing_return_type;

        auto special_definition = false;
        while (!is_declaration_end(stream, special_definition))
        {
            assert(!stream.done());

            if (detail::skip_if_token(stream, ")"))
            {
                // return type was a function pointer
                // now come the arguments
                trailing_return_type += ")(";

                skip_bracket_count(stream, "(", ")",
                                [&](const char *str)
                                {
                                    trailing_return_type += str;
                                });
            }
            else if (detail::skip_if_token(stream, "->"))
            {
                // trailing return type
                while (!is_declaration_end(stream, special_definition))
                {
                    auto spelling = stream.get().get_value();
                    trailing_return_type += spelling.c_str();
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
                finfo.explicit_noexcept = true;
                finfo.noexcept_expression = parse_noexcept(stream);
            }
            else
                assert(std::isspace(stream.get().get_value()[0]));
        }

        if (special_definition)
        {
            auto res = parse_special_definition(stream);
            if (res == cpp_function_definition_normal)
                minfo.virtual_flag = cpp_virtual_pure;
            else
                finfo.definition = res;
        }

        return trailing_return_type;
    }

    cpp_type_ref parse_member_function(detail::token_stream &stream, cpp_cursor cur, const cpp_name &name,
                                        cpp_function_info &finfo, cpp_member_function_info &minfo)
    {
        auto type = clang_getCursorResultType(cur);

        skip_template_parameter_declaration(stream);

        auto return_type = parse_member_function_prefix(stream, name, finfo, minfo);

        skip_template_arguments(stream);

        // handle parameters
        auto variadic = false;
        skip_parameters(stream, variadic);
        if (variadic)
            finfo.set_flag(cpp_variadic_fnc);

        return_type += parse_member_function_suffix(stream, finfo, minfo);
        if (return_type.empty())
        {
            // we have a deduced return type
            return_type = "auto";
        }
        else
        {
            while (std::isspace(return_type.back()))
                return_type.pop_back();
        }

        if (finfo.noexcept_expression.empty())
            finfo.noexcept_expression = "false";

        return {type, std::move(return_type)};
    }

    void parse_parameters(translation_unit &tu, cpp_function_base *base, cpp_cursor cur)
    {
        // we cannot use clang_Cursor_getNumArguments(),
        // doesn't work for templates
        // luckily parameters are exposed as children nodes
        // but if returning a function pointer, its parameters are as well
        // so obtain number of return parameters
        // and ignore those

        auto type = clang_getPointeeType(clang_getCursorResultType(cur));
        auto no_params_return = clang_getNumArgTypes(type);
        if (no_params_return == -1)
            no_params_return = 0;

        auto i = 0;
        detail::visit_children(cur, [&](CXCursor cur, CXCursor)
        {
            if (clang_getCursorKind(cur) == CXCursor_ParmDecl && i++ >= no_params_return)
                base->add_parameter(cpp_function_parameter::parse(tu, cur));
            return CXChildVisit_Continue;
        });
    }
}

cpp_ptr<cpp_function> cpp_function::parse(translation_unit &tu, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_FunctionDecl
          || clang_getTemplateCursorKind(cur) == CXCursor_FunctionDecl);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    auto name = detail::parse_name(cur);
    cpp_function_info finfo;
    cpp_member_function_info minfo;

    auto return_type = parse_member_function(stream, cur, name, finfo, minfo);
    assert(minfo.virtual_flag == cpp_virtual_none || minfo.virtual_flag == cpp_virtual_static);
    assert(minfo.cv_qualifier == cpp_cv_none);
    assert(minfo.ref_qualifier == cpp_ref_none);

    auto result = detail::make_ptr<cpp_function>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                                 std::move(return_type), std::move(finfo));

    parse_parameters(tu, result.get(), cur);

    return result;
}

cpp_ptr<cpp_member_function> cpp_member_function::parse(translation_unit &tu, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_CXXMethod
           || clang_getTemplateCursorKind(cur) == CXCursor_CXXMethod);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    auto name = detail::parse_name(cur);
    cpp_function_info finfo;
    cpp_member_function_info minfo;

    auto return_type = parse_member_function(stream, cur, name, finfo, minfo);

    auto result = detail::make_ptr<cpp_member_function>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                                        std::move(return_type),
                                                        std::move(finfo), std::move(minfo));

    parse_parameters(tu, result.get(), cur);

    return result;
}

namespace
{
    cpp_name parse_conversion_op_name(cpp_cursor cur, cpp_type_ref &type)
    {
        cpp_name name;

        if (clang_getCursorKind(cur) == CXCursor_ConversionFunction)
        {
            // parse name
            name = detail::parse_name(cur);

            auto target_type = clang_getCursorResultType(cur);
            auto target_type_spelling = name.substr(9); // take everything from type after "operator "
            assert(target_type_spelling.front() != ' '); // no multiple whitespace

            type = cpp_type_ref(target_type, std::move(target_type_spelling));
        }
        else if (clang_getCursorKind(cur) == CXCursor_FunctionTemplate)
        {
            // parsing
            // template <typename T> operator T();
            // yields a name of
            // operator type-parameter-0-0
            // so workaround by calculating name from the type spelling
            auto target_type = clang_getCursorResultType(cur);
            auto spelling = detail::parse_name(target_type);
            name = "operator " + spelling;

            type = cpp_type_ref(target_type, spelling);
        }

        return name;
    }
}

cpp_ptr<cpp_conversion_op> cpp_conversion_op::parse(translation_unit &tu, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_ConversionFunction
           || clang_getTemplateCursorKind(cur) == CXCursor_ConversionFunction);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    cpp_type_ref type;
    auto name = parse_conversion_op_name(cur, type);

    cpp_function_info finfo;
    cpp_member_function_info minfo;

    skip_template_parameter_declaration(stream);

    // handle prefix
    while (!detail::skip_if_token(stream, "operator"))
    {
        if (detail::skip_if_token(stream, "explicit"))
            finfo.set_flag(cpp_explicit_conversion);
        else if (detail::skip_if_token(stream, "constexpr"))
            finfo.set_flag(cpp_constexpr_fnc);
        else if (detail::skip_if_token(stream, "virtual"))
            minfo.virtual_flag = cpp_virtual_new;
        else
            assert(std::isspace(stream.get().get_value()[0]));
    }

    // skip until parameters
    while (stream.peek().get_value() != "(" && stream.peek().get_value() != "<")
        stream.bump();

    skip_template_arguments(stream);

    auto variadic = false;
    skip_parameters(stream, variadic);
    assert(!variadic);

    auto trailing_return_type = parse_member_function_suffix(stream, finfo, minfo);
    assert(trailing_return_type.empty());

    if (finfo.noexcept_expression.empty())
        finfo.noexcept_expression = "false";

    return detail::make_ptr<cpp_conversion_op>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                               type, std::move(finfo), std::move(minfo));
}

cpp_ptr<cpp_constructor> cpp_constructor::parse(translation_unit &tu, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_Constructor
           || clang_getTemplateCursorKind(cur) == CXCursor_Constructor);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    auto name = detail::parse_name(cur);
    cpp_function_info info;

    skip_template_parameter_declaration(stream);

    // handle prefix
    while (!detail::skip_if_token(stream, name.c_str()))
    {
        if (detail::skip_if_token(stream, "explicit"))
            info.set_flag(cpp_explicit_conversion);
        else if (detail::skip_if_token(stream, "constexpr"))
            info.set_flag(cpp_constexpr_fnc);
        else
            assert(std::isspace(stream.get().get_value()[0]));
    }

    skip_template_arguments(stream);

    // handle parameters
    auto variadic = false;
    skip_parameters(stream, variadic);
    if (variadic)
        info.set_flag(cpp_variadic_fnc);

    // parse suffix
    auto special_definition = false;
    while (!is_declaration_end(stream, special_definition))
    {
        assert(!stream.done());

        if (detail::skip_if_token(stream, "noexcept"))
        {
            info.explicit_noexcept = true;
            info.noexcept_expression = parse_noexcept(stream);
        }
        else
            assert(std::isspace(stream.get().get_value()[0]));
    }

    // parse special definition
    if (special_definition)
    {
        info.definition = parse_special_definition(stream);
        assert(info.definition != cpp_function_definition_normal);
    }

    if (!info.explicit_noexcept)
        info.noexcept_expression = "false";

    auto result =  detail::make_ptr<cpp_constructor>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                                     std::move(info));
    parse_parameters(tu, result.get(), cur);
    return result;
}

cpp_ptr<cpp_destructor> cpp_destructor::parse(translation_unit &tu, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_Destructor
           || clang_getTemplateCursorKind(cur) == CXCursor_Destructor);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    auto name = detail::parse_name(cur);
    cpp_function_info info;
    auto virtual_flag = cpp_virtual_none;

    if (detail::skip_if_token(stream, "virtual"))
        virtual_flag = cpp_virtual_new;
    else if (detail::skip_if_token(stream, "constexpr"))
        info.set_flag(cpp_constexpr_fnc);

    // skip name and arguments
    detail::skip(stream, {"~", &name[1], "(", ")"});

    // parse suffix
    auto special_definition = false;
    while (!is_declaration_end(stream, special_definition))
    {
        assert(!stream.done());

        if (detail::skip_if_token(stream, "final"))
            virtual_flag = cpp_virtual_final;
        else if (detail::skip_if_token(stream, "override"))
            virtual_flag = cpp_virtual_overriden;
        else if (detail::skip_if_token(stream, "noexcept"))
        {
            info.explicit_noexcept = true;
            info.noexcept_expression = parse_noexcept(stream);
        }
        else
            assert(std::isspace(stream.get().get_value()[0]));
    }

    // parse special definition
    if (special_definition)
    {
        auto res = parse_special_definition(stream);
        if (res == cpp_function_definition_normal)
            virtual_flag = cpp_virtual_pure;
        else
            info.definition = res;
    }

    // dtors are implictly noexcept
    if (!info.explicit_noexcept)
        info.noexcept_expression = "true";

    return detail::make_ptr<cpp_destructor>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                            std::move(info), virtual_flag);
}
