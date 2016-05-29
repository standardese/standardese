// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_SYNOPSIS_UTILS_HPP_INCLUDED
#define STANDARDESE_DETAIL_SYNOPSIS_UTILS_HPP_INCLUDED

#include <standardese/cpp_type.hpp>
#include <standardese/output.hpp>

namespace standardese
{
    class cpp_class;
    class cpp_function_base;
} // namespace standardese

namespace standardese { namespace detail
{
    template <class Container, typename T, typename Func>
    void write_range(output_base::code_block_writer &out,
                     const Container &cont, T sep,
                     Func f)
    {
        auto need = false;
        for (auto &e : cont)
        {
            if (need)
                out << sep;

            need = f(out, e);
        }
    }

    void write_type_value_default(output_base::code_block_writer &out,
                                  const cpp_type_ref &type, const cpp_name &name,
                                  const std::string &def = "");

    void write_class_name(output_base::code_block_writer &out,
                          const cpp_name &name, int class_type);

    void write_bases(output_base::code_block_writer &out, const cpp_class &c, bool extract_private);

    void write_parameters(output_base::code_block_writer &out, const cpp_function_base &f,
                          const cpp_name &override_name);

    void write_noexcept(output_base::code_block_writer &out, const cpp_function_base &f);

    void write_definition(output_base::code_block_writer &out, const cpp_function_base &f, bool pure = false);

    void write_cv_ref(output_base::code_block_writer &out, int cv, int ref);

    void write_prefix(output_base::code_block_writer &out, int virtual_flag, bool constexpr_f, bool explicit_f = false);

    void write_override_final(output_base::code_block_writer &out, int virtual_flag);
}} // namespace standardese::detail

#endif // STANDARDESE_SYNOPSIS_UTILS_HPP_INCLUDED
