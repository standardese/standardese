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
        // erases template arguments
        void clean_name(cpp_name &name);

        // obtains the name from cursor
        cpp_name parse_name(cpp_cursor cur);

        // otbains the name of a type
        cpp_name parse_name(CXType type);

        // parses the class name from cursor
        // cannot use parse_name(), adds "struct"/... before
        cpp_name parse_class_name(cpp_cursor cur);

        // obtains the comment from cursor
        cpp_raw_comment parse_comment(cpp_cursor cur);

        // calculates the full scope name of a cursor (without trailing ::)
        cpp_name parse_scope(cpp_cursor cur);

        // when concatenating tokens for the default values
        // template <typename T = foo<int>> yields foo<int>>
        // because >> is one token
        // count brackets, if unbalanced, remove final >
        void unmunch(std::string &str);

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
