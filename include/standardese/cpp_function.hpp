// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_FUNCTION_HPP_INCLUDED
#define STANDARDESE_CPP_FUNCTION_HPP_INCLUDED

#include <string>

#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_type.hpp>

namespace standardese
{
    class cpp_function_parameter
    : public cpp_parameter_base
    {
    public:
        static cpp_ptr<cpp_function_parameter> parse(translation_unit &tu,  cpp_cursor cur);

        cpp_function_parameter(cpp_name name, cpp_raw_comment comment,
                               cpp_type_ref type, std::string default_value = "")
        : cpp_parameter_base(function_parameter_t, std::move(name), std::move(comment)),
          type_(std::move(type)), default_(std::move(default_value)) {}

        const cpp_type_ref& get_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

        bool has_default_value() const STANDARDESE_NOEXCEPT
        {
            return !default_.empty();
        }

        const std::string& get_default_value() const STANDARDESE_NOEXCEPT
        {
            return default_;
        }

    private:
        cpp_type_ref type_;
        std::string  default_;
    };

    enum cpp_function_flags : unsigned
    {
        cpp_variadic_fnc        = 1,
        cpp_constexpr_fnc       = 2,
        cpp_explicit_conversion = 4,
    };

    enum cpp_function_definition
    {
        cpp_function_definition_normal,
        cpp_function_definition_deleted,
        cpp_function_definition_defaulted,
    };

    struct cpp_function_info
    {
        cpp_function_flags flags = cpp_function_flags(0);
        cpp_function_definition definition = cpp_function_definition_normal;
        std::string noexcept_expression;
        bool explicit_noexcept = false;

        void set_flag(cpp_function_flags f) STANDARDESE_NOEXCEPT
        {
            flags = cpp_function_flags(unsigned(flags) | unsigned(f));
        }
    };

    // common stuff for all functions
    class cpp_function_base
    : public cpp_entity, private cpp_entity_container<cpp_function_parameter>
    {
    public:
        static cpp_ptr<standardese::cpp_function_base> try_parse(translation_unit &tu, cpp_name scope,
                                                                        cpp_cursor cur);

        void add_parameter(cpp_ptr<cpp_function_parameter> param)
        {
            cpp_entity_container::add_entity(std::move(param));
        }

        const cpp_entity_container<cpp_function_parameter>& get_parameters() const STANDARDESE_NOEXCEPT
        {
            return *this;
        }

        bool is_variadic() const STANDARDESE_NOEXCEPT
        {
            return info_.flags & cpp_variadic_fnc;
        }

        bool is_constexpr() const STANDARDESE_NOEXCEPT
        {
            return info_.flags & cpp_constexpr_fnc;
        }

        bool is_explicit() const STANDARDESE_NOEXCEPT
        {
            return info_.flags & cpp_explicit_conversion;
        }

        cpp_function_definition get_definition() const STANDARDESE_NOEXCEPT
        {
            return info_.definition;
        }

        // the part inside a noexcept(...)
        // noexcept without condition leads to "true"
        // no noexcept at all leads to "false"
        const std::string& get_noexcept() const STANDARDESE_NOEXCEPT
        {
            return info_.noexcept_expression;
        }

        bool explicit_noexcept() const STANDARDESE_NOEXCEPT
        {
            return info_.explicit_noexcept;
        }

    protected:
        cpp_function_base(cpp_entity::type t,
                          cpp_name scope, cpp_name name, cpp_raw_comment comment,
                          cpp_function_info info)
        : cpp_entity(t, std::move(scope), std::move(name), std::move(comment)),
          info_(std::move(info)) {}

    private:
        cpp_function_info info_;
    };

    class cpp_function
    : public cpp_function_base
    {
    public:
        static cpp_ptr<cpp_function> parse(translation_unit &tu,  cpp_name scope, cpp_cursor cur);

        cpp_function(cpp_name scope, cpp_name name, cpp_raw_comment comment,
                     cpp_type_ref return_type, cpp_function_info info)
        : cpp_function_base(function_t,
                            std::move(scope), std::move(name), std::move(comment),
                            std::move(info)),
          return_(std::move(return_type)) {}

        const cpp_type_ref& get_return_type() const STANDARDESE_NOEXCEPT
        {
            return return_;
        }

    private:
        cpp_type_ref return_;
    };

    enum cpp_cv
    {
        cpp_cv_none     = 0,
        cpp_cv_const    = 1,
        cpp_cv_volatile = 2,
    };

    inline bool is_const(cpp_cv cv) STANDARDESE_NOEXCEPT
    {
        return cv & cpp_cv_const;
    }

    inline bool is_volatile(cpp_cv cv) STANDARDESE_NOEXCEPT
    {
        return cv & cpp_cv_volatile;
    }

    enum cpp_ref_qualifier
    {
        cpp_ref_none,
        cpp_ref_lvalue,
        cpp_ref_rvalue
    };

    enum cpp_virtual
    {
        cpp_virtual_static,
        cpp_virtual_none,
        cpp_virtual_pure,
        cpp_virtual_new,
        cpp_virtual_overriden,
        cpp_virtual_final
    };

    inline bool is_virtual(cpp_virtual virt) STANDARDESE_NOEXCEPT
    {
        return virt != cpp_virtual_static && virt != cpp_virtual_none;
    }

    inline bool is_overriden(cpp_virtual virt) STANDARDESE_NOEXCEPT
    {
        return virt == cpp_virtual_overriden || virt == cpp_virtual_final;
    }

    struct cpp_member_function_info
    {
        cpp_cv cv_qualifier = cpp_cv(0);
        cpp_ref_qualifier ref_qualifier = cpp_ref_none;
        cpp_virtual virtual_flag = cpp_virtual_none;

        void set_cv(cpp_cv cv) STANDARDESE_NOEXCEPT
        {
            cv_qualifier = cpp_cv(unsigned(cv_qualifier) | unsigned(cv));
        }
    };

    class cpp_member_function
    : public cpp_function
    {
    public:
        static cpp_ptr<cpp_member_function> parse(translation_unit &tu,  cpp_name scope, cpp_cursor cur);

        cpp_member_function(cpp_name scope, cpp_name name, cpp_raw_comment comment,
                            cpp_type_ref return_type,
                            cpp_function_info finfo, cpp_member_function_info minfo)
        : cpp_function(std::move(scope), std::move(name), std::move(comment),
                       std::move(return_type), std::move(finfo)),
          info_(minfo)
        {
            set_type(member_function_t);
        }

        cpp_cv get_cv() const STANDARDESE_NOEXCEPT
        {
            return info_.cv_qualifier;
        }

        cpp_ref_qualifier get_ref_qualifier() const STANDARDESE_NOEXCEPT
        {
            return info_.ref_qualifier;
        }

        cpp_virtual get_virtual() const STANDARDESE_NOEXCEPT
        {
            return info_.virtual_flag;
        }

    private:
        cpp_member_function_info info_;
    };

    class cpp_conversion_op
    : public cpp_function_base
    {
    public:
        static cpp_ptr<cpp_conversion_op> parse(translation_unit &tu,  cpp_name scope, cpp_cursor cur);

        cpp_conversion_op(cpp_name scope, cpp_name name, cpp_raw_comment comment,
                          cpp_type_ref target_type,
                          cpp_function_info finfo, cpp_member_function_info minfo)
        : cpp_function_base(conversion_op_t,
                            std::move(scope), std::move(name), std::move(comment),
                            std::move(finfo)),
          target_type_(std::move(target_type)), info_(minfo) {}

        const cpp_type_ref& get_target_type() const STANDARDESE_NOEXCEPT
        {
            return target_type_;
        }

        cpp_cv get_cv() const STANDARDESE_NOEXCEPT
        {
            return info_.cv_qualifier;
        }

        cpp_ref_qualifier get_ref_qualifier() const STANDARDESE_NOEXCEPT
        {
            return info_.ref_qualifier;
        }

        cpp_virtual get_virtual() const STANDARDESE_NOEXCEPT
        {
            return info_.virtual_flag;
        }

    private:
        cpp_type_ref target_type_;
        cpp_member_function_info info_;
    };

    class cpp_constructor
    : public cpp_function_base
    {
    public:
        static cpp_ptr<cpp_constructor> parse(translation_unit &tu,  cpp_name scope, cpp_cursor cur);

        cpp_constructor(cpp_name scope, cpp_name name, cpp_raw_comment comment,
                        cpp_function_info info)
        : cpp_function_base(constructor_t,
                            std::move(scope), std::move(name), std::move(comment),
                            std::move(info)) {}
    };

    class cpp_destructor
    : public cpp_function_base
    {
    public:
        static cpp_ptr<cpp_destructor> parse(translation_unit &tu,  cpp_name scope, cpp_cursor cur);

        cpp_destructor(cpp_name scope, cpp_name name, cpp_raw_comment comment,
                       cpp_function_info info, cpp_virtual virtual_flag)
        : cpp_function_base(destructor_t,
                            std::move(scope), std::move(name), std::move(comment),
                            std::move(info)),
          virtual_(virtual_flag) {}

        cpp_virtual get_virtual() const STANDARDESE_NOEXCEPT
        {
            return virtual_;
        }

    private:
        cpp_virtual virtual_;
    };

    namespace detail
    {
        inline bool is_virtual(const cpp_entity &e)
        {
            if (e.get_entity_type() == cpp_entity::member_function_t)
                return standardese::is_virtual(static_cast<const cpp_member_function&>(e).get_virtual());
            else if (e.get_entity_type() == cpp_entity::conversion_op_t)
                return standardese::is_virtual(static_cast<const cpp_conversion_op&>(e).get_virtual());
            else if (e.get_entity_type() == cpp_entity::destructor_t)
                return standardese::is_virtual(static_cast<const cpp_destructor&>(e).get_virtual());

            return false;
        }
    } // namespace detail
} // namespace standardese

#endif // STANDARDESE_CPP_FUNCTION_HPP_INCLUDED
