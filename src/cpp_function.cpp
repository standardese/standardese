// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_function.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>

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

cpp_ptr<cpp_function_parameter> cpp_function_parameter::parse(cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_ParmDecl);

    auto name = detail::parse_name(cur);
    std::string default_value;
    auto type = parse_parameter_type(cur, name, default_value);

    return detail::make_ptr<cpp_function_parameter>(std::move(name), detail::parse_comment(cur),
                                                    std::move(type), std::move(default_value));
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

        return {type, std::move(type_name)};
    }

    void parse_parameters(cpp_function_base *base, cpp_cursor cur)
    {
        auto no = clang_Cursor_getNumArguments(cur);
        for (auto i = 0; i != no; ++i)
            base->add_parameter(cpp_function_parameter::parse(clang_Cursor_getArgument(cur, i)));
    }
}

cpp_ptr<cpp_function> cpp_function::parse(cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_FunctionDecl);

    auto name = detail::parse_name(cur);
    cpp_function_info info;
    auto return_type = parse_function_info(cur, name, info);

    auto result = detail::make_ptr<cpp_function>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                                 std::move(return_type), std::move(info));

    parse_parameters(result.get(), cur);

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

        return {type, std::move(type_name)};
    }
}

cpp_ptr<cpp_member_function> cpp_member_function::parse(cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_CXXMethod);

    auto name = detail::parse_name(cur);
    cpp_function_info finfo;
    cpp_member_function_info minfo;
    auto return_type = parse_member_function_info(cur, name, finfo, minfo);

    auto result = detail::make_ptr<cpp_member_function>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                                        std::move(return_type),
                                                        std::move(finfo), std::move(minfo));

    parse_parameters(result.get(), cur);

    return result;
}

cpp_ptr<cpp_conversion_op> cpp_conversion_op::parse(cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_ConversionFunction);

    auto name = detail::parse_name(cur);

    cpp_function_info finfo;
    cpp_member_function_info minfo;
    auto return_type = detail::parse_function_info(cur, name, finfo, minfo);
    assert(return_type.empty());

    auto target_type = clang_getCursorResultType(cur);
    auto target_type_spelling = name.substr(9);
    assert(target_type_spelling.front() != ' '); // no multiple whitespace

    return detail::make_ptr<cpp_conversion_op>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                               cpp_type_ref(target_type, std::move(target_type_spelling)),
                                               std::move(finfo), std::move(minfo));
}

cpp_ptr<cpp_constructor> cpp_constructor::parse(cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_Constructor);

    auto name = detail::parse_name(cur);

    cpp_function_info info;
    auto return_type = parse_function_info(cur, name, info);
    assert(return_type.get_name().empty());

    auto result =  detail::make_ptr<cpp_constructor>(std::move(scope), std::move(name), detail::parse_comment(cur),
                                                     std::move(info));
    parse_parameters(result.get(), cur);
    return result;
}
