// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_TYPE_HPP_INCLUDED
#define STANDARDESE_CPP_TYPE_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>

namespace standardese
{
    class cpp_type
    : public cpp_entity
    {
    protected:
        cpp_type(cpp_name scope, cpp_name name, cpp_comment comment)
        : cpp_entity(std::move(scope), std::move(name), std::move(comment)) {}
    };
} // namespace standardese

#endif // STANDARDESE_CPP_TYPE_HPP_INCLUDED
