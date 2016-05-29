// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_function.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/error.hpp>

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
        auto location = source_location(clang_getCursorLocation(cur), name);

        for (auto in_type = true; stream.peek().get_value() != ";";)
        {
            detail::skip_attribute(stream, location);
            if (detail::skip_if_token(stream, name.c_str()))
                continue;
            else if (detail::skip_if_token(stream, "="))
                in_type = false;
            else
                (in_type ? type_name : default_value) += stream.get().get_value().c_str();
        }

        while (std::isspace(type_name.back()))
            type_name.pop_back();

        while (std::isspace(default_value.back()))
            default_value.pop_back();

        return {type, std::move(type_name)};
    }
}

cpp_ptr<cpp_function_parameter> cpp_function_parameter::parse(translation_unit &tu, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_ParmDecl);

    auto name = detail::parse_name(cur);
    std::string default_value;
    auto type = parse_parameter_type(tu, cur, name, default_value);

    return detail::make_ptr<cpp_function_parameter>(std::move(name),
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
    void skip_template_parameter_declaration(detail::token_stream &stream, const source_location &location)
    {
        if (stream.peek().get_value() == "template")
        {
            stream.bump();
            skip_bracket_count(stream, location, "<", ">");
            detail::skip_whitespace(stream);
        }
    }

    cpp_name parse_member_function_prefix(detail::token_stream &stream, const source_location &location,
                                          const cpp_name &name,
                                          cpp_function_info &finfo, cpp_member_function_info &minfo)
    {
        cpp_name return_type;
        auto allow_auto = false; // whether or not auto is allowed in return type

        while (!detail::skip_if_token(stream, name.c_str()))
        {
            detail::skip_attribute(stream, location);

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
                const char *ptr = &name[std::strlen("operator")];
                while (true)
                {
                    while (std::isspace(*ptr))
                        ++ptr;

                    auto& spelling = stream.peek().get_value();
                    if (!std::isspace(spelling[0]))
                    {
                        auto res = std::strncmp(ptr, spelling.c_str(), spelling.size());
                        if (res != 0)
                            break;
                        ptr += spelling.size();
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
                return_type += spelling.c_str(); // part of return type
            }
        }

        return return_type;
    }

    void skip_template_arguments(detail::token_stream &stream, const source_location &location)
    {
        if (stream.peek().get_value() == "<")
            skip_bracket_count(stream, location, "<", ">");
    }

    void skip_parameters(detail::token_stream &stream, const source_location &location, bool &variadic)
    {
        variadic = false;

        // whether or not a variadic parameter can come
        // i.e. after first bracket or comma
        auto variadic_param = true;
        skip_bracket_count(stream, location, "(", ")",
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
        if (stream.peek().get_value().size() > 1u)
            return false;

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
    cpp_function_definition parse_special_definition(detail::token_stream &stream, const source_location &location)
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

        throw parse_error(location, std::string("unknown function definition \'= ") + spelling.c_str() + "\'");
    }

    cpp_name parse_member_function_suffix(detail::token_stream &stream, const source_location &location,
                                    cpp_function_info &finfo, cpp_member_function_info &minfo)
    {
        cpp_name trailing_return_type;

        auto special_definition = false;
        while (!is_declaration_end(stream, special_definition))
        {
            assert(!stream.done());
            detail::skip_attribute(stream, location);

            if (detail::skip_if_token(stream, ")"))
            {
                // return type was a function pointer
                // now come the arguments
                trailing_return_type += ")(";

                skip_bracket_count(stream, location, "(", ")",
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
            else if (!std::isspace(stream.peek().get_value()[0]))
            {
                auto str = stream.get().get_value();
                throw parse_error(location, "unexpected token \'" + std::string(str.c_str()) + "\'");
            }
            else
                // is whitespace, so consume
                stream.get();
        }

        if (special_definition)
        {
            auto res = parse_special_definition(stream, location);
            if (res == cpp_function_definition_normal)
                minfo.virtual_flag = cpp_virtual_pure;
            else
                finfo.definition = res;
        }

        return trailing_return_type;
    }

    cpp_type_ref parse_member_function(detail::token_stream &stream, const source_location &location,
                                       cpp_cursor cur, const cpp_name &name,
                                       cpp_function_info &finfo, cpp_member_function_info &minfo)
    {
        auto type = clang_getCursorResultType(cur);

        skip_template_parameter_declaration(stream, location);

        auto return_type = parse_member_function_prefix(stream, location, name, finfo, minfo);

        skip_template_arguments(stream, location);

        // handle parameters
        auto variadic = false;
        skip_parameters(stream, location, variadic);
        if (variadic)
            finfo.set_flag(cpp_variadic_fnc);

        return_type += parse_member_function_suffix(stream, location, finfo, minfo);
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

    source_location location(clang_getCursorLocation(cur), name);

    auto return_type = parse_member_function(stream, location, cur, name, finfo, minfo);
    if (is_virtual(minfo.virtual_flag))
        throw parse_error(location, "virtual specifier on normal function");
    if (minfo.cv_qualifier != cpp_cv_none)
        throw parse_error(location, "cv qualifier on normal function");
    if (minfo.ref_qualifier != cpp_ref_none)
        throw parse_error(location, "ref qualifier on normal function");

    auto result = detail::make_ptr<cpp_function>(std::move(scope), std::move(name),
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

    source_location location(clang_getCursorLocation(cur), name);

    auto return_type = parse_member_function(stream, location, cur, name, finfo, minfo);

    auto result = detail::make_ptr<cpp_member_function>(std::move(scope), std::move(name),
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

    source_location location(clang_getCursorLocation(cur), name);

    cpp_function_info finfo;
    cpp_member_function_info minfo;

    skip_template_parameter_declaration(stream, location);

    // handle prefix
    while (!detail::skip_if_token(stream, "operator"))
    {
        detail::skip_attribute(stream, location);

        if (detail::skip_if_token(stream, "explicit"))
            finfo.set_flag(cpp_explicit_conversion);
        else if (detail::skip_if_token(stream, "constexpr"))
            finfo.set_flag(cpp_constexpr_fnc);
        else if (detail::skip_if_token(stream, "virtual"))
            minfo.virtual_flag = cpp_virtual_new;
        else if (!std::isspace(stream.peek().get_value()[0]))
        {
            auto str = stream.get().get_value();
            throw parse_error(location, "unexpected token \'" + std::string(str.c_str()) + "\'");
        }
        else
            // is whitespace, so consume
            stream.get();
    }

    // skip until parameters
    while (stream.peek().get_value() != "(" && stream.peek().get_value() != "<")
        stream.bump();

    skip_template_arguments(stream, location);

    auto variadic = false;
    skip_parameters(stream, location, variadic);
    if (variadic)
        throw parse_error(location, "conversion op is variadic");

    auto trailing_return_type = parse_member_function_suffix(stream, location, finfo, minfo);
    if (!trailing_return_type.empty())
        throw parse_error(location, "conversion op has trailing return type");

    if (finfo.noexcept_expression.empty())
        finfo.noexcept_expression = "false";

    return detail::make_ptr<cpp_conversion_op>(std::move(scope), std::move(name),
                                               type, std::move(finfo), std::move(minfo));
}

cpp_ptr<cpp_constructor> cpp_constructor::parse(translation_unit &tu, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_Constructor
           || clang_getTemplateCursorKind(cur) == CXCursor_Constructor);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    auto name = detail::parse_name(cur);
    detail::clean_name(name);
    cpp_function_info info;

    source_location location(clang_getCursorLocation(cur), name);

    skip_template_parameter_declaration(stream, location);

    // handle prefix
    while (!detail::skip_if_token(stream, name.c_str()))
    {
        detail::skip_attribute(stream, location);

        if (detail::skip_if_token(stream, "explicit"))
            info.set_flag(cpp_explicit_conversion);
        else if (detail::skip_if_token(stream, "constexpr"))
            info.set_flag(cpp_constexpr_fnc);
        else if (!std::isspace(stream.peek().get_value()[0]))
        {
            auto str = stream.get().get_value();
            throw parse_error(location, "unexpected token \'" + std::string(str.c_str()) + "\'");
        }
        else
            // is whitespace, so consume
            stream.get();
    }

    skip_template_arguments(stream, location);

    // handle parameters
    auto variadic = false;
    skip_parameters(stream, location, variadic);
    if (variadic)
        info.set_flag(cpp_variadic_fnc);

    // parse suffix
    auto special_definition = false;
    while (!is_declaration_end(stream, special_definition))
    {
        assert(!stream.done());
        detail::skip_attribute(stream, location);

        if (detail::skip_if_token(stream, "noexcept"))
        {
            info.explicit_noexcept = true;
            info.noexcept_expression = parse_noexcept(stream);
        }
        else if (!std::isspace(stream.peek().get_value()[0]))
        {
            auto str = stream.get().get_value();
            throw parse_error(location, "unexpected token \'" + std::string(str.c_str()) + "\'");
        }
        else
            // is whitespace, so consume
            stream.get();
    }

    // parse special definition
    if (special_definition)
    {
        info.definition = parse_special_definition(stream, location);
        if (info.definition == cpp_function_definition_normal)
            throw parse_error(location, "constructor is pure virtual");
    }

    if (!info.explicit_noexcept)
        info.noexcept_expression = "false";

    auto result =  detail::make_ptr<cpp_constructor>(std::move(scope), std::move(name),
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
    detail::clean_name(name);
    cpp_function_info info;
    auto virtual_flag = cpp_virtual_none;

    source_location location(clang_getCursorLocation(cur), name);

    if (detail::skip_if_token(stream, "virtual"))
        virtual_flag = cpp_virtual_new;
    else if (detail::skip_if_token(stream, "constexpr"))
        info.set_flag(cpp_constexpr_fnc);

    detail::skip_attribute(stream, location);
    detail::skip_whitespace(stream);

    // skip name and arguments
    detail::skip(stream, location, {"~", &name[1], "(", ")"});

    // parse suffix
    auto special_definition = false;
    while (!is_declaration_end(stream, special_definition))
    {
        assert(!stream.done());
        detail::skip_attribute(stream, location);

        if (detail::skip_if_token(stream, "final"))
            virtual_flag = cpp_virtual_final;
        else if (detail::skip_if_token(stream, "override"))
            virtual_flag = cpp_virtual_overriden;
        else if (detail::skip_if_token(stream, "noexcept"))
        {
            info.explicit_noexcept = true;
            info.noexcept_expression = parse_noexcept(stream);
        }
        else if (!std::isspace(stream.peek().get_value()[0]))
        {
            auto str = stream.get().get_value();
            throw parse_error(location, "unexpected token \'" + std::string(str.c_str()) + "\'");
        }
        else
            // is whitespace, so consume
            stream.get();
    }

    // parse special definition
    if (special_definition)
    {
        auto res = parse_special_definition(stream, location);
        if (res == cpp_function_definition_normal)
            virtual_flag = cpp_virtual_pure;
        else
            info.definition = res;
    }

    // dtors are implictly noexcept
    if (!info.explicit_noexcept)
        info.noexcept_expression = "true";

    return detail::make_ptr<cpp_destructor>(std::move(scope), std::move(name),
                                            std::move(info), virtual_flag);
}
