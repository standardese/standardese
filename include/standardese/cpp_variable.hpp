// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_VARIABLE_HPP_INCLUDED
#define STANDARDESE_CPP_VARIABLE_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_type.hpp>

namespace standardese
{
    enum cpp_linkage
    {
        cpp_no_linkage,
        cpp_internal_linkage,
        cpp_external_linkage,
    };

    class cpp_variable
    : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_variable> parse(cpp_name scope, cpp_cursor cur);

        cpp_variable(cpp_name scope, cpp_name name, cpp_comment comment,
                     cpp_type_ref type, std::string initializer,
                     cpp_linkage linkage = cpp_no_linkage,
                     bool is_thread_local = false)
        : cpp_entity(std::move(scope), std::move(name), std::move(comment)),
          type_(std::move(type)), initializer_(std::move(initializer)),
          linkage_(std::move(linkage)),
          thread_local_(is_thread_local) {}

        const cpp_type_ref& get_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

        const std::string& get_initializer() const STANDARDESE_NOEXCEPT
        {
            return initializer_;
        }

        cpp_linkage get_linkage() const STANDARDESE_NOEXCEPT
        {
            return linkage_;
        }

        bool is_thread_local() const STANDARDESE_NOEXCEPT
        {
            return thread_local_;
        }

    private:
        cpp_type_ref type_;
        std::string initializer_;
        cpp_linkage linkage_;
        bool thread_local_;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_VARIABLE_HPP_INCLUDED
