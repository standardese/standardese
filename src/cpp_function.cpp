// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_function.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/string.hpp>

using namespace standardese;

namespace
{
    cpp_type_ref parse_parameter_type(cpp_cursor cur, const cpp_name &name,
                                      std::string &default_value)
    {
        assert(clang_getCursorKind(cur) == CXCursor_ParmDecl);

        auto type = clang_getCursorType(cur);
        auto type_name = detail::parse_variable_type_name(cur, name, default_value);

        return {type, std::move(type_name)};
    }
}

cpp_ptr<cpp_function_parameter> cpp_function_parameter::parse(const parser &, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_ParmDecl);

    auto name = detail::parse_name(cur);
    std::string default_value;
    auto type = parse_parameter_type(cur, name, default_value);

    return detail::make_ptr<cpp_function_parameter>(std::move(name), detail::parse_comment(cur),
                                                    std::move(type), std::move(default_value));
}

cpp_ptr<standardese::cpp_function_base> cpp_function_base::try_parse(const parser &p, cpp_name scope,
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
    cpp_type_ref parse_function_info(cpp_cursor cur, const cpp_name &name,
                                     cpp_function_info &info)
    {
        auto type = clang_getCursorResultType(cur);

        cpp_member_function_info minfo;
        auto type_name = detail::parse_function_info(cur, name, info, minfo);
        assert(minfo.virtual_flag  == cpp_virtual_none);
        assert(minfo.cv_qualifier  == cpp_cv(0));
        assert(minfo.ref_qualifier == cpp_ref_none);

        // no noexcept
        if (info.noexcept_expression.empty())
            info.noexcept_expression = "false";

        return {type, std::move(type_name)};
    }

    void parse_parameters(const parser &p, cpp_function_base *base, cpp_cursor cur)
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
                base->add_parameter(cpp_function_parameter::parse(p, cur));
            return CXChildVisit_Continue;
        });
    }
}

cpp_ptr<cpp_function> cpp_function::parse(const parser &p, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_FunctionDecl
          || clang_getTemplateCursorKind(cur) == CXCursor_FunctionDecl);

    auto name = detail::parse_name(cur);
    cpp_function_info info;
    auto return_type = parse_function_info(cur, name, info);

    auto result = detail::make_ptr<cpp_function>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                                 std::move(return_type), std::move(info));

    parse_parameters(p, result.get(), cur);

    return result;
}

namespace
{
    cpp_type_ref parse_member_function_info(cpp_cursor cur, const cpp_name &name,
                                            cpp_function_info &finfo,
                                            cpp_member_function_info &minfo)
    {
        auto type = clang_getCursorResultType(cur);
        auto type_name = detail::parse_function_info(cur, name, finfo, minfo);

        // no noexcept
        if (finfo.noexcept_expression.empty())
            finfo.noexcept_expression = "false";

        // handle static functions
        if (clang_CXXMethod_isStatic(cur))
        {
            assert(minfo.virtual_flag == cpp_virtual_none);
            minfo.virtual_flag = cpp_virtual_static;
        }

        return {type, std::move(type_name)};
    }
}

cpp_ptr<cpp_member_function> cpp_member_function::parse(const parser &p, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_CXXMethod
           || clang_getTemplateCursorKind(cur) == CXCursor_CXXMethod);

    auto name = detail::parse_name(cur);
    cpp_function_info finfo;
    cpp_member_function_info minfo;
    auto return_type = parse_member_function_info(cur, name, finfo, minfo);

    auto result = detail::make_ptr<cpp_member_function>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                                        std::move(return_type),
                                                        std::move(finfo), std::move(minfo));

    parse_parameters(p, result.get(), cur);

    return result;
}

cpp_ptr<cpp_conversion_op> cpp_conversion_op::parse(const parser &, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_ConversionFunction
           || clang_getTemplateCursorKind(cur) == CXCursor_ConversionFunction);

    cpp_name name;
    cpp_type_ref type;
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

    cpp_function_info finfo;
    cpp_member_function_info minfo;
    auto return_type = detail::parse_function_info(cur, name, finfo, minfo);
    assert(return_type.empty());
    // no noexcept
    if (finfo.noexcept_expression.empty())
        finfo.noexcept_expression = "false";

    return detail::make_ptr<cpp_conversion_op>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                               type, std::move(finfo), std::move(minfo));
}

cpp_ptr<cpp_constructor> cpp_constructor::parse(const parser &p, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_Constructor
           || clang_getTemplateCursorKind(cur) == CXCursor_Constructor);

    auto name = detail::parse_name(cur);

    cpp_function_info info;
    auto return_type = parse_function_info(cur, name, info);
    assert(return_type.get_name().empty());
    // no noexcept
    if (info.noexcept_expression.empty())
        info.noexcept_expression = "false";

    auto result =  detail::make_ptr<cpp_constructor>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                                     std::move(info));
    parse_parameters(p, result.get(), cur);
    return result;
}

cpp_ptr<cpp_destructor> cpp_destructor::parse(const parser &, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_Destructor
           || clang_getTemplateCursorKind(cur) == CXCursor_Destructor);

    auto name = detail::parse_name(cur);

    cpp_function_info info;
    cpp_member_function_info minfo;
    auto return_type = detail::parse_function_info(cur, name, info, minfo);
    assert(return_type.empty());
    assert(minfo.cv_qualifier == cpp_cv(0));
    assert(minfo.ref_qualifier == cpp_ref_none);

    // no noexcept
    if (info.noexcept_expression.empty())
        info.noexcept_expression = "true"; // destructors are implicitly noexcept!

    return detail::make_ptr<cpp_destructor>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                            std::move(info), minfo.virtual_flag);
}
