// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_PARSE_UTILS_HPP_INCLUDED
#define STANDARDESE_PARSE_UTILS_HPP_INCLUDED

#include <string>

#include <standardese/cpp_cursor.hpp>
#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_function.hpp>

namespace standardese
{
    struct cpp_function_info;
    struct cpp_member_function_info;

    namespace detail
    {
        // obtains the name from cursor
        cpp_name parse_name(cpp_cursor cur);

        // parses the class name from cursor
        // cannot use parse_name(), adds "struct"/... before
        cpp_name parse_class_name(cpp_cursor cur);

        // obtains the comment from cursor
        cpp_comment parse_comment(cpp_cursor cur);

        // calculates the full scope name of a cursor (without trailing ::)
        cpp_name parse_scope(cpp_cursor cur);

        // parses the name of a typedef type
        cpp_name parse_typedef_type_name(cpp_cursor cur, const cpp_name &name);

        // parses the name of a variable type
        // also provides initializer
        cpp_name parse_variable_type_name(cpp_cursor cur, const cpp_name &name, std::string &initializer);

        // parses the name of a C++ alias
        cpp_name parse_alias_type_name(cpp_cursor cur);

        // parses the name of the underlying type of an enum
        cpp_name parse_enum_type_name(cpp_cursor cur, bool &definition);

        // parses function information
        // returns the name of the return type
        cpp_name parse_function_info(cpp_cursor cur, const cpp_name &name,
                                     cpp_function_info &finfo,
                                     cpp_member_function_info &minfo);

        // parses the default type of a C++ template type parameter
        cpp_name parse_template_type_default(cpp_cursor cur, bool &variadic);

        // parses type and name of a C++ non type template parameter
        cpp_name parse_template_non_type_type(cpp_cursor cur, const cpp_name &name,
                                              std::string &def, bool &variadic);

        // parses name of a template specialization
        cpp_name parse_template_specialization_name(cpp_cursor cur, const cpp_name &name);

        // parses the replacement of a macro
        std::string parse_macro_replacement(cpp_cursor cur, const cpp_name &name, std::string &args);

        // wrapper for clang_visitChildren
        template <typename Fnc>
        void visit_children(cpp_cursor cur, Fnc f)
        {
            auto cb = [](CXCursor cur, CXCursor parent, CXClientData data)
            -> CXChildVisitResult
            {
                return (*static_cast<Fnc*>(data))(cur, parent);
            };
            clang_visitChildren(cur, cb, &f);
        }
    } // namespace detail
} // namespace standardese

#endif // STANDARDESE_PARSE_UTILS_HPP_INCLUDED
