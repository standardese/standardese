// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_GET_SPECIAL_ENTITY_HPP_INCLUDED
#define STANDARDESE_GET_SPECIAL_ENTITY_HPP_INCLUDED

#include <cppast/cpp_class.hpp>
#include <cppast/cpp_file.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_preprocessor.hpp>
#include <cppast/cpp_template.hpp>

namespace standardese
{
namespace detail
{
    inline type_safe::optional_ref<const cppast::cpp_macro_definition> get_macro(
        const cppast::cpp_entity& e)
    {
        if (e.kind() == cppast::cpp_macro_definition::kind())
            return type_safe::ref(static_cast<const cppast::cpp_macro_definition&>(e));
        else
            return type_safe::nullopt;
    }

    inline type_safe::optional_ref<const cppast::cpp_function_base> get_function(
        const cppast::cpp_entity& e)
    {
        if (cppast::is_function(e.kind()))
            return type_safe::ref(static_cast<const cppast::cpp_function_base&>(e));
        else if (cppast::is_template(e.kind()))
            return get_function(*static_cast<const cppast::cpp_template&>(e).begin());
        else
            return type_safe::nullopt;
    }

    inline type_safe::optional_ref<const cppast::cpp_template> get_template(
        const cppast::cpp_entity& e)
    {
        if (cppast::is_template(e.kind()))
            return type_safe::ref(static_cast<const cppast::cpp_template&>(e));
        else if (cppast::is_templated(e))
            return get_template(e.parent().value());
        else
            return type_safe::nullopt;
    }

    inline type_safe::optional_ref<const cppast::cpp_class> get_class(const cppast::cpp_entity& e)
    {
        if (e.kind() == cppast::cpp_class::kind())
            return type_safe::ref(static_cast<const cppast::cpp_class&>(e));
        else if (cppast::is_template(e.kind()))
            return get_class(*static_cast<const cppast::cpp_template&>(e).begin());
        else
            return type_safe::nullopt;
    }
} // namespace detail
} // namespace standardese

#endif // STANDARDESE_GET_SPECIAL_ENTITY_HPP_INCLUDED
