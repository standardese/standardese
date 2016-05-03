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

        cpp_variable(cpp_name scope, cpp_name name, cpp_raw_comment comment,
                     cpp_type_ref type, std::string initializer,
                     cpp_linkage linkage = cpp_no_linkage,
                     bool is_thread_local = false)
        : cpp_entity(variable_t, std::move(scope),
                     std::move(name), std::move(comment)),
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

    class cpp_member_variable
    : public cpp_variable
    {
    public:
        static cpp_ptr<cpp_member_variable> parse(cpp_name scope, cpp_cursor cur);

        cpp_member_variable(cpp_name scope, cpp_name name, cpp_raw_comment comment,
                         cpp_type_ref type, std::string initializer,
                         cpp_linkage linkage = cpp_no_linkage,
                         bool is_mutable = false, bool is_thread_local = false)
        : cpp_variable(std::move(scope), std::move(name), std::move(comment),
                       std::move(type), std::move(initializer), linkage,
                       is_thread_local),
          mutable_(is_mutable)
        {
            set_type(member_variable_t);
        }

        bool is_mutable() const STANDARDESE_NOEXCEPT
        {
            return mutable_;
        }

    private:
        bool mutable_;
    };

    class cpp_bitfield
    : public cpp_member_variable
    {
    public:
        cpp_bitfield(cpp_name scope, cpp_name name, cpp_raw_comment comment,
                    cpp_type_ref type, std::string initializer, unsigned no,
                    cpp_linkage linkage = cpp_no_linkage,
                    bool is_mutable = false, bool is_thread_local = false)
        : cpp_member_variable(std::move(scope), std::move(name), std::move(comment),
                            std::move(type), std::move(initializer), linkage,
                            is_mutable, is_thread_local),
          no_bits_(no)
        {
            set_type(bitfield_t);
        }

        unsigned no_bits() const STANDARDESE_NOEXCEPT
        {
            return no_bits_;
        }

    private:
        unsigned no_bits_;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_VARIABLE_HPP_INCLUDED
