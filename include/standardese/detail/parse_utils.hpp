// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_PARSE_UTILS_HPP_INCLUDED
#define STANDARDESE_PARSE_UTILS_HPP_INCLUDED

#include <string>

#include <standardese/cpp_cursor.hpp>
#include <standardese/cpp_entity.hpp>

namespace standardese
{
    namespace detail
    {
        // obtains the name from cursor
        cpp_name parse_name(cpp_cursor cur);

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
        cpp_name parse_enum_type_name(cpp_cursor cur);

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
