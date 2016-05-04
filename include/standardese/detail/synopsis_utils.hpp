// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_SYNOPSIS_UTILS_HPP_INCLUDED
#define STANDARDESE_DETAIL_SYNOPSIS_UTILS_HPP_INCLUDED

#include <standardese/cpp_type.hpp>
#include <standardese/output.hpp>

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
            else
                need = true;

            f(out, e);
        }
    }

    void write_type_value_default(output_base::code_block_writer &out,
                                  const cpp_type_ref &type, const cpp_name &name,
                                  const std::string &def = "");
}} // namespace standardese::detail

#endif // STANDARDESE_SYNOPSIS_UTILS_HPP_INCLUDED
