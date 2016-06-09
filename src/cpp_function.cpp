// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_function.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/cpp_class.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/translation_unit.hpp>
#include <clang-c/Index.h>

using namespace standardese;

cpp_ptr<cpp_function_parameter> cpp_function_parameter::parse(translation_unit &tu, cpp_cursor cur,
                                                              const cpp_entity &parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_ParmDecl);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);
    auto name = detail::parse_name(cur);

    std::string type_name, default_value;
    for (auto in_type = true; stream.peek().get_value() != ";";)
    {
        detail::skip_attribute(stream, cur);
        if (detail::skip_if_token(stream, name.c_str()))
            continue;
        else if (detail::skip_if_token(stream, "="))
            in_type = false;
        else
            (in_type ? type_name : default_value) += stream.get().get_value().c_str();
    }

    detail::erase_trailing_ws(type_name);
    detail::erase_trailing_ws(default_value);

    return detail::make_ptr<cpp_function_parameter>(cur, parent,
                                                    cpp_type_ref(std::move(type_name), clang_getCursorType(cur)),
                                                    std::move(default_value));
}

cpp_ptr<cpp_function_base> cpp_function_base::try_parse(translation_unit &p, cpp_cursor cur, const cpp_entity &parent)
{
    auto kind = clang_getCursorKind(cur);
    if (kind == CXCursor_FunctionTemplate)
        kind = clang_getTemplateCursorKind(cur);

    switch (kind)
    {
        case CXCursor_FunctionDecl:
            return cpp_function::parse(p, cur, parent);
        case CXCursor_CXXMethod:
            return cpp_member_function::parse(p, cur, parent);
        case CXCursor_ConversionFunction:
            return cpp_conversion_op::parse(p, cur, parent);
        case CXCursor_Constructor:
            return cpp_constructor::parse(p, cur, parent);
        case CXCursor_Destructor:
            return cpp_destructor::parse(p, cur, parent);
        default:
            break;
    }

    return nullptr;
}

namespace
{
    void skip_template_parameter_declaration(detail::token_stream &stream, cpp_cursor cur)
    {
        if (stream.peek().get_value() == "template")
        {
            stream.bump();
            skip_bracket_count(stream, cur, "<", ">");
            detail::skip_whitespace(stream);
        }
    }

    std::string parse_member_function_prefix(detail::token_stream &stream, cpp_cursor cur,
                                             const cpp_name &name,
                                             cpp_function_info &finfo, cpp_member_function_info &minfo)
    {
        std::string return_type;
        auto allow_auto = false; // whether or not auto is allowed in return type

        while (!detail::skip_if_token(stream, name.c_str()))
        {
            detail::skip_attribute(stream, cur);

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
                const char *ptr = name.c_str() + std::strlen("operator");
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

    void skip_template_arguments(detail::token_stream &stream, cpp_cursor cur)
    {
        if (stream.peek().get_value() == "<")
            skip_bracket_count(stream, cur, "<", ">");
    }

    void skip_parameters(detail::token_stream &stream, cpp_cursor cur, bool &variadic)
    {
        variadic = false;

        // whether or not a variadic parameter can come
        // i.e. after first bracket or comma
        auto variadic_param = true;
        skip_bracket_count(stream, cur, "(", ")",
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

    bool is_declaration_end(detail::token_stream &stream, cpp_cursor cur, bool &is_special_definition)
    {
        detail::skip_attribute(stream, cur);

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
    cpp_function_definition parse_special_definition(detail::token_stream &stream, cpp_cursor cur)
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

        throw parse_error(source_location(cur),
                          std::string("unknown function definition \'= ") + spelling.c_str() + "\'");
    }

    std::string parse_member_function_suffix(detail::token_stream &stream, cpp_cursor cur,
                                             cpp_function_info &finfo, cpp_member_function_info &minfo)
    {
        std::string trailing_return_type;

        auto special_definition = false;
        while (!is_declaration_end(stream, cur, special_definition))
        {
            assert(!stream.done());
            detail::skip_attribute(stream, cur);

            if (detail::skip_if_token(stream, ")"))
            {
                // return type was a function pointer
                // now come the arguments
                trailing_return_type += ")(";

                skip_bracket_count(stream, cur, "(", ")",
                                   [&](const char *str)
                                   {
                                       trailing_return_type += str;
                                   });
            }
            else if (detail::skip_if_token(stream, "->"))
            {
                // trailing return type
                while (!is_declaration_end(stream, cur, special_definition))
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
                throw parse_error(source_location(cur), "unexpected token \'" + std::string(str.c_str()) + "\'");
            }
            else
                // is whitespace, so consume
                stream.get();
        }

        if (special_definition)
        {
            auto res = parse_special_definition(stream, cur);
            if (res == cpp_function_definition_normal)
                minfo.virtual_flag = cpp_virtual_pure;
            else
                finfo.definition = res;
        }

        return trailing_return_type;
    }

    cpp_type_ref parse_member_function(detail::token_stream &stream,
                                       cpp_cursor cur, const cpp_name &name,
                                       cpp_function_info &finfo, cpp_member_function_info &minfo)
    {
        skip_template_parameter_declaration(stream, cur);

        auto return_type = parse_member_function_prefix(stream, cur, name, finfo, minfo);

        skip_template_arguments(stream, cur);

        // handle parameters
        auto variadic = false;
        skip_parameters(stream, cur, variadic);
        if (variadic)
            finfo.set_flag(cpp_variadic_fnc);

        return_type += parse_member_function_suffix(stream, cur, finfo, minfo);
        if (return_type.empty())
        {
            // we have a deduced return type
            return_type = "auto";
        }
        else
            detail::erase_trailing_ws(return_type);

        if (finfo.noexcept_expression.empty())
            finfo.noexcept_expression = "false";

        return {std::move(return_type), clang_getCursorResultType(cur)};
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
                base->add_parameter(cpp_function_parameter::parse(tu, cur, *base));
            return CXChildVisit_Continue;
        });
    }
}

cpp_ptr<cpp_function> cpp_function::parse(translation_unit &tu,
                                          cpp_cursor cur, const cpp_entity &parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_FunctionDecl
           || clang_getTemplateCursorKind(cur) == CXCursor_FunctionDecl);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);
    auto name = detail::parse_name(cur);

    cpp_function_info finfo;
    cpp_member_function_info minfo;
    auto return_type = parse_member_function(stream, cur, name, finfo, minfo);
    if (is_virtual(minfo.virtual_flag))
        throw parse_error(source_location(cur), "virtual specifier on normal function");
    if (minfo.cv_qualifier != cpp_cv_none)
        throw parse_error(source_location(cur), "cv qualifier on normal function");
    if (minfo.ref_qualifier != cpp_ref_none)
        throw parse_error(source_location(cur), "ref qualifier on normal function");

    auto result = detail::make_ptr<cpp_function>(cur, parent, std::move(return_type), std::move(finfo));
    parse_parameters(tu, result.get(), cur);
    return result;
}

namespace
{


    bool is_implicit_virtual(translation_unit &tu,
                             const cpp_function_base &func,
                             const cpp_class &cur_base);

    bool check_bases(translation_unit &tu, const cpp_function_base &func,
                    const cpp_class &c)
    {
        auto result = false;
        for (auto& base : c.get_bases())
        {
            cpp_ptr<cpp_class> cptr;
            auto cur_base = base.get_class(tu.get_registry());
            if (!cur_base)
            {
                if (base.get_type().is_invalid())
                    continue;

                auto decl = clang_getTypeDeclaration(clang_getCanonicalType(base.get_type().get_cxtype()));
                if (!clang_isDeclaration(clang_getCursorKind(decl)))
                    continue;

                cptr = cpp_class::parse(tu, decl, tu.get_file());
                if (!cptr)
                    // happens in a template class when the base is a primary template without definition
                    continue;

                detail::visit_children(cptr->get_cursor(), [&](cpp_cursor cur, cpp_cursor)
                {
                    // skip unnecessary cursors
                    // only need the three functions that can be virtual
                    // as well as further base classes
                    auto kind = clang_getCursorKind(cur);
                    if (kind != CXCursor_CXXMethod
                        && kind != CXCursor_ConversionFunction
                        && kind != CXCursor_Destructor
                        && kind != CXCursor_CXXBaseSpecifier)
                        return CXChildVisit_Continue;

                    auto e = cpp_entity::try_parse(tu, cur, *cptr);
                    if (!e)
                        return CXChildVisit_Continue;
                    cptr->add_entity(std::move(e));
                    return CXChildVisit_Continue;
                });
                cur_base = cptr.get();
            }

            assert(cur_base);
            result = is_implicit_virtual(tu, func, *cur_base);
            if (result)
                return result;
        }

        return result;
    }

    bool compare_parameter(const cpp_function_base &a, const cpp_function_base &b)
    {
        auto a_begin = a.get_parameters().begin();
        auto a_end = a.get_parameters().end();
        auto b_begin = b.get_parameters().begin();
        auto b_end = b.get_parameters().end();

        while (a_begin != a_end && b_begin != b_end)
        {
            auto& cur_a = *a_begin;
            auto& cur_b = *b_begin;
            if (cur_a.get_name() != cur_b.get_name())
                return false;
            else if (cur_a.get_type() != cur_b.get_type())
                return false;

            ++a_begin;
            ++b_begin;
        }

        return (a_begin == a_end) == (b_begin == b_end);

    }

    bool can_be_overriden(const cpp_member_function &a, const cpp_member_function &b)
    {
        if (!is_virtual(a.get_virtual()) || a.get_virtual() == cpp_virtual_final)
            return false;

        if (a.is_variadic() != b.is_variadic())
            return false;
        else if (a.get_cv() != b.get_cv())
            return false;
        else if (a.get_ref_qualifier() != b.get_ref_qualifier())
            return false;
        else if (a.get_return_type() != b.get_return_type())
            return false;
        else if (a.get_name() != b.get_name())
            return false;
        else if (a.get_noexcept() != b.get_noexcept())
            return false;

        return compare_parameter(a, b);
    }

    bool can_be_overriden(const cpp_conversion_op &a, const cpp_conversion_op &b)
    {
        if (!is_virtual(a.get_virtual()) || a.get_virtual() == cpp_virtual_final)
            return false;

        if (a.is_explicit() != b.is_explicit())
            return false;
        else if (a.get_cv() != b.get_cv())
            return false;
        else if (a.get_ref_qualifier() != b.get_ref_qualifier())
            return false;
        else if (a.get_target_type() != b.get_target_type())
            return false;
        else if (a.get_name() != b.get_name())
            return false;
        else if (a.get_noexcept() != b.get_noexcept())
            return false;

        return true;
    }

    bool can_be_overriden(const cpp_destructor &a, const cpp_destructor &b)
    {
        if (!is_virtual(a.get_virtual()) || a.get_virtual() == cpp_virtual_final)
            return false;

        return a.get_noexcept() == b.get_noexcept();
    }

    bool can_be_overriden(const cpp_function_base &a, const cpp_function_base &b)
    {
        assert(a.get_entity_type() == b.get_entity_type());

        if (a.get_entity_type() == cpp_entity::member_function_t)
            return can_be_overriden(static_cast<const cpp_member_function&>(a),
                                    static_cast<const cpp_member_function&>(b));
        else if (a.get_entity_type() == cpp_entity::conversion_op_t)
            return can_be_overriden(static_cast<const cpp_conversion_op&>(a),
                                    static_cast<const cpp_conversion_op&>(b));
        else if (a.get_entity_type() == cpp_entity::destructor_t)
            return can_be_overriden(static_cast<const cpp_destructor&>(a),
                                    static_cast<const cpp_destructor&>(b));

        assert(false);
        return false;
    }

    bool is_implicit_virtual(translation_unit &tu,
                             const cpp_function_base &func,
                             const cpp_class &cur_base)
    {
        for (auto& member : cur_base)
        {
            if (member.get_entity_type() == func.get_entity_type()
                && can_be_overriden(static_cast<const cpp_function_base&>(member), func))
                return true;
        }

        return check_bases(tu, func, cur_base);
    }

    bool is_implicit_virtual(translation_unit &tu, const cpp_function_base &func)
    {
        assert(func.get_entity_type() != cpp_entity::function_t
                && func.get_entity_type() != cpp_entity::constructor_t);
        if (func.is_constexpr())
            return cpp_virtual_none;

        auto parent = get_class(func.get_parent());
        assert(parent);
        return check_bases(tu, func, *parent);
    }
}

cpp_ptr<cpp_member_function> cpp_member_function::parse(translation_unit &tu,
                                                        cpp_cursor cur, const cpp_entity &parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_CXXMethod
           || clang_getTemplateCursorKind(cur) == CXCursor_CXXMethod);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);
    auto name = detail::parse_name(cur);

    cpp_function_info finfo;
    cpp_member_function_info minfo;
    auto return_type = parse_member_function(stream, cur, name, finfo, minfo);

    auto result = detail::make_ptr<cpp_member_function>(cur, parent, std::move(return_type),
                                                        std::move(finfo), std::move(minfo));
    parse_parameters(tu, result.get(), cur);

    if ((result->get_virtual() == cpp_virtual_none
         || result->get_virtual() == cpp_virtual_new)
        && is_implicit_virtual(tu, *result))
        // check for implicit virtual
        result->info_.virtual_flag = cpp_virtual_overriden;

    return result;
}

namespace
{
    cpp_type_ref parse_conversion_op_type(cpp_cursor cur)
    {
        if (clang_getCursorKind(cur) == CXCursor_ConversionFunction)
        {
            // parse name
            std::string name = detail::parse_name(cur).c_str();

            auto target_type = clang_getCursorResultType(cur);
            auto target_type_spelling = name.substr(9); // take everything from type after "operator "
            assert(target_type_spelling.front() != ' '); // no multiple whitespace

            return cpp_type_ref(std::move(target_type_spelling), target_type);
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

            return cpp_type_ref(spelling, target_type);
        }

        assert(false);
        throw parse_error(source_location(cur), "internal error");
    }
}

cpp_ptr<cpp_conversion_op> cpp_conversion_op::parse(translation_unit &tu, cpp_cursor cur, const cpp_entity &parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_ConversionFunction
           || clang_getTemplateCursorKind(cur) == CXCursor_ConversionFunction);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    auto type = parse_conversion_op_type(cur);

    cpp_function_info finfo;
    cpp_member_function_info minfo;

    skip_template_parameter_declaration(stream, cur);

    // handle prefix
    while (!detail::skip_if_token(stream, "operator"))
    {
        detail::skip_attribute(stream, cur);

        if (detail::skip_if_token(stream, "explicit"))
            finfo.set_flag(cpp_explicit_conversion);
        else if (detail::skip_if_token(stream, "constexpr"))
            finfo.set_flag(cpp_constexpr_fnc);
        else if (detail::skip_if_token(stream, "virtual"))
            minfo.virtual_flag = cpp_virtual_new;
        else if (!std::isspace(stream.peek().get_value()[0]))
        {
            auto str = stream.get().get_value();
            throw parse_error(source_location(cur), "unexpected token \'" + std::string(str.c_str()) + "\'");
        }
        else
            // is whitespace, so consume
            stream.get();
    }

    // skip until parameters
    while (stream.peek().get_value() != "(" && stream.peek().get_value() != "<")
        stream.bump();

    skip_template_arguments(stream, cur);

    auto variadic = false;
    skip_parameters(stream, cur, variadic);
    if (variadic)
        throw parse_error(source_location(cur), "conversion op is variadic");

    auto trailing_return_type = parse_member_function_suffix(stream, cur, finfo, minfo);
    if (!trailing_return_type.empty())
        throw parse_error(source_location(cur), "conversion op has trailing return type");

    if (finfo.noexcept_expression.empty())
        finfo.noexcept_expression = "false";

    auto result = detail::make_ptr<cpp_conversion_op>(cur, parent,
                                               std::move(type), std::move(finfo), std::move(minfo));
    if ((result->get_virtual() == cpp_virtual_none
         || result->get_virtual() == cpp_virtual_new)
        && is_implicit_virtual(tu, *result))
        // check for implicit virtual
        result->info_.virtual_flag = cpp_virtual_overriden;
    return result;
}

cpp_name cpp_conversion_op::get_name() const
{
    return std::string("operator ") + target_type_.get_name().c_str();
}

cpp_ptr<cpp_constructor> cpp_constructor::parse(translation_unit &tu, cpp_cursor cur, const cpp_entity &parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_Constructor
           || clang_getTemplateCursorKind(cur) == CXCursor_Constructor);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    std::string name = detail::parse_name(cur).c_str();
    detail::erase_template_args(name);

    skip_template_parameter_declaration(stream, cur);

    // handle prefix
    cpp_function_info info;
    while (!detail::skip_if_token(stream, name.c_str()))
    {
        detail::skip_attribute(stream, cur);

        if (detail::skip_if_token(stream, "explicit"))
            info.set_flag(cpp_explicit_conversion);
        else if (detail::skip_if_token(stream, "constexpr"))
            info.set_flag(cpp_constexpr_fnc);
        else if (!std::isspace(stream.peek().get_value()[0]))
        {
            auto str = stream.get().get_value();
            throw parse_error(source_location(cur), "unexpected token \'" + std::string(str.c_str()) + "\'");
        }
        else
            // is whitespace, so consume
            stream.get();
    }

    skip_template_arguments(stream, cur);

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
            info.explicit_noexcept = true;
            info.noexcept_expression = parse_noexcept(stream);
        }
        else if (!std::isspace(stream.peek().get_value()[0]))
        {
            auto str = stream.get().get_value();
            throw parse_error(source_location(cur), "unexpected token \'" + std::string(str.c_str()) + "\'");
        }
        else
            // is whitespace, so consume
            stream.get();
    }

    // parse special definition
    if (special_definition)
    {
        info.definition = parse_special_definition(stream, cur);
        if (info.definition == cpp_function_definition_normal)
            throw parse_error(source_location(cur), "constructor is pure virtual");
    }

    if (!info.explicit_noexcept)
        info.noexcept_expression = "false";

    auto result =  detail::make_ptr<cpp_constructor>(cur, parent, std::move(info));
    parse_parameters(tu, result.get(), cur);
    return result;
}

cpp_name cpp_constructor::get_name() const
{
    std::string str = cpp_entity::get_name().c_str();
    detail::erase_template_args(str);
    return str;
}

cpp_ptr<cpp_destructor> cpp_destructor::parse(translation_unit &tu, cpp_cursor cur, const cpp_entity &parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_Destructor
           || clang_getTemplateCursorKind(cur) == CXCursor_Destructor);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);

    std::string name = detail::parse_name(cur).c_str();
    detail::erase_template_args(name);

    cpp_function_info info;
    auto virtual_flag = cpp_virtual_none;
    if (detail::skip_if_token(stream, "virtual"))
        virtual_flag = cpp_virtual_new;
    else if (detail::skip_if_token(stream, "constexpr"))
        info.set_flag(cpp_constexpr_fnc);

    detail::skip_attribute(stream, cur);
    detail::skip_whitespace(stream);

    // skip name and arguments
    detail::skip(stream, cur, {"~", &name[1], "(", ")"});

    // parse suffix
    auto special_definition = false;
    while (!is_declaration_end(stream, cur, special_definition))
    {
        assert(!stream.done());
        detail::skip_attribute(stream, cur);

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
            throw parse_error(source_location(cur), "unexpected token \'" + std::string(str.c_str()) + "\'");
        }
        else
            // is whitespace, so consume
            stream.get();
    }

    // parse special definition
    if (special_definition)
    {
        auto res = parse_special_definition(stream, cur);
        if (res == cpp_function_definition_normal)
            virtual_flag = cpp_virtual_pure;
        else
            info.definition = res;
    }

    // dtors are implictly noexcept
    if (!info.explicit_noexcept)
        info.noexcept_expression = "true";

    auto result = detail::make_ptr<cpp_destructor>(cur, parent, std::move(info), virtual_flag);
    if ((result->get_virtual() == cpp_virtual_none
         || result->get_virtual() == cpp_virtual_new)
        && is_implicit_virtual(tu, *result))
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
