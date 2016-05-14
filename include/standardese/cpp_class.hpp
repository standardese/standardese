// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_CLASS_HPP_INCLUDED
#define STANDARDESE_CPP_CLASS_HPP_INCLUDED

#include <standardese/cpp_type.hpp>

namespace standardese
{
    enum cpp_access_specifier_t
    {
        cpp_private,
        cpp_protected,
        cpp_public
    };

    const char* to_string(cpp_access_specifier_t access) STANDARDESE_NOEXCEPT;

    class cpp_access_specifier
    : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_access_specifier> parse(const parser &p,  cpp_cursor cur);

        cpp_access_specifier(cpp_access_specifier_t a)
        : cpp_entity(access_specifier_t, "", to_string(a), ""), access_(a) {}

        cpp_access_specifier_t get_access() const STANDARDESE_NOEXCEPT
        {
            return access_;
        }

    private:
        cpp_access_specifier_t access_;
    };

    class cpp_base_class
    : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_base_class> parse(const parser &p,  cpp_name scope, cpp_cursor cur);

        cpp_base_class(cpp_name scope,
                       cpp_name name, CXType type,
                       cpp_access_specifier_t access,
                       bool is_virtual)
        : cpp_entity(base_class_t, std::move(scope), std::move(name), ""), type_(type), access_(access),
          virtual_(is_virtual) {}

        CXType get_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

        cpp_access_specifier_t get_access() const STANDARDESE_NOEXCEPT
        {
            return access_;
        }

        bool is_virtual() const STANDARDESE_NOEXCEPT
        {
            return virtual_;
        }

    private:
        CXType type_;
        cpp_access_specifier_t access_;
        bool virtual_;
    };

    enum cpp_class_type
    {
        cpp_class_t,
        cpp_struct_t,
        cpp_union_t
    };

    class cpp_class
    : public cpp_type, public cpp_entity_container<cpp_entity>
    {
    public:
        class parser : public cpp_entity_parser
        {
        public:
            parser(const standardese::parser &p, cpp_name scope, cpp_cursor cur);

            void add_entity(cpp_entity_ptr ptr) override;

            cpp_name scope_name() override;

            cpp_entity_ptr finish(const standardese::parser &par) override;

        private:
            cpp_ptr<cpp_class> class_;
        };

        cpp_class(cpp_name scope, cpp_name name, cpp_raw_comment comment,
                  CXType type, cpp_class_type ctype, bool is_final)
        : cpp_type(class_t, std::move(scope), std::move(name), std::move(comment), type),
          type_(ctype), final_(is_final) {}

        void add_entity(cpp_entity_ptr e)
        {
            cpp_entity_container::add_entity(std::move(e));
        }

        cpp_class_type get_class_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

        bool is_final() const STANDARDESE_NOEXCEPT
        {
            return final_;
        }

    private:
        cpp_class_type type_;
        bool final_;

        friend parser;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_CLASS_HPP_INCLUDED
