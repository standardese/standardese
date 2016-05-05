// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_SYNOPSIS_HPP_INCLUDED
#define STANDARDESE_SYNOPSIS_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>
#include <standardese/output.hpp>

namespace standardese
{
    void write_synopsis(output_base &out, const cpp_entity &e);
} // namespace standardese

#endif // STANDARDESE_SYNOPSIS_HPP_INCLUDED
