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
    class cpp_function_parameter final : public cpp_entity
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::function_parameter_t;
        }

        static cpp_ptr<cpp_function_parameter> parse(translation_unit& tu, cpp_cursor cur,
                                                     const cpp_entity& parent);

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
        cpp_function_parameter(cpp_cursor cur, const cpp_entity& parent, cpp_type_ref type,
                               std::string def)
        : cpp_entity(get_entity_type(), cur, parent),
          type_(std::move(type)),
          default_(std::move(def))
        {
        }

        cpp_type_ref type_;
        std::string  default_;

        friend detail::cpp_ptr_access;
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
        cpp_function_flags      flags      = cpp_function_flags(0);
        cpp_function_definition definition = cpp_function_definition_normal;
        std::string             noexcept_expression;
        bool                    explicit_noexcept = false;

        void set_flag(cpp_function_flags f) STANDARDESE_NOEXCEPT
        {
            flags = cpp_function_flags(unsigned(flags) | unsigned(f));
        }
    };

    // common stuff for all functions
    class cpp_function_base : public cpp_entity,
                              private cpp_entity_container<cpp_function_parameter>
    {
    public:
        static cpp_ptr<cpp_function_base> try_parse(translation_unit& tu, cpp_cursor cur,
                                                    const cpp_entity& parent,
                                                    unsigned          template_offset = 0);

        void add_parameter(cpp_ptr<cpp_function_parameter> param)
        {
            cpp_entity_container<cpp_function_parameter>::add_entity(std::move(param));
        }

        const cpp_entity_container<cpp_function_parameter>& get_parameters() const
            STANDARDESE_NOEXCEPT
        {
            return *this;
        }

        cpp_name get_scope() const override;

        bool is_variadic() const STANDARDESE_NOEXCEPT
        {
            return !!(info_.flags & cpp_variadic_fnc);
        }

        bool is_constexpr() const STANDARDESE_NOEXCEPT
        {
            return !!(info_.flags & cpp_constexpr_fnc);
        }

        bool is_explicit() const STANDARDESE_NOEXCEPT
        {
            return !!(info_.flags & cpp_explicit_conversion);
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

        /// \returns The signature of a function,
        /// i.e. the parameter types and everything else that influences overload resolution.
        virtual cpp_name get_signature() const = 0;

    protected:
        cpp_function_base(cpp_entity::type t, cpp_cursor cur, const cpp_entity& parent,
                          cpp_function_info info)
        : cpp_entity(t, cur, parent), info_(std::move(info))
        {
        }

        void set_template_specialization_name(cpp_name name);

    private:
        cpp_name do_get_unique_name() const override
        {
            return std::string(get_full_name().c_str()) + get_signature().c_str();
        }

        cpp_function_info info_;
    };

    class cpp_function final : public cpp_function_base
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::function_t;
        }

        static cpp_ptr<cpp_function> parse(translation_unit& tu, cpp_cursor cur,
                                           const cpp_entity& parent, unsigned template_offset = 0);

        const cpp_type_ref& get_return_type() const STANDARDESE_NOEXCEPT
        {
            return return_;
        }

        cpp_name get_signature() const override
        {
            return signature_;
        }

    private:
        cpp_function(cpp_cursor cur, const cpp_entity& parent, cpp_type_ref ret,
                     cpp_function_info info);

        cpp_type_ref return_;
        std::string  signature_;

        friend detail::cpp_ptr_access;
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
        return !!(cv & cpp_cv_volatile);
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
        cpp_cv            cv_qualifier  = cpp_cv(0);
        cpp_ref_qualifier ref_qualifier = cpp_ref_none;
        cpp_virtual       virtual_flag  = cpp_virtual_none;

        void set_cv(cpp_cv cv) STANDARDESE_NOEXCEPT
        {
            cv_qualifier = cpp_cv(unsigned(cv_qualifier) | unsigned(cv));
        }
    };

    class cpp_member_function final : public cpp_function_base
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::member_function_t;
        }

        static cpp_ptr<cpp_member_function> parse(translation_unit& tu, cpp_cursor cur,
                                                  const cpp_entity& parent,
                                                  unsigned          template_offset = 0);

        const cpp_type_ref& get_return_type() const STANDARDESE_NOEXCEPT
        {
            return return_;
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

        cpp_name get_signature() const override
        {
            return signature_;
        }

    private:
        cpp_member_function(cpp_cursor cur, const cpp_entity& parent, cpp_type_ref ret,
                            cpp_function_info finfo, cpp_member_function_info minfo);

        cpp_type_ref             return_;
        cpp_member_function_info info_;
        std::string              signature_;

        friend detail::cpp_ptr_access;
    };

    class cpp_conversion_op final : public cpp_function_base
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::conversion_op_t;
        }

        static cpp_ptr<cpp_conversion_op> parse(translation_unit& tu, cpp_cursor cur,
                                                const cpp_entity& parent,
                                                unsigned          template_offset = 0);

        cpp_name get_name() const override;

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

        cpp_name get_signature() const override;

    private:
        cpp_conversion_op(cpp_cursor cur, const cpp_entity& parent, cpp_type_ref target,
                          cpp_function_info finfo, cpp_member_function_info minfo)
        : cpp_function_base(get_entity_type(), cur, parent, std::move(finfo)),
          target_type_(std::move(target)),
          info_(std::move(minfo))
        {
        }

        cpp_type_ref             target_type_;
        cpp_member_function_info info_;

        friend detail::cpp_ptr_access;
    };

    class cpp_constructor final : public cpp_function_base
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::constructor_t;
        }

        static cpp_ptr<cpp_constructor> parse(translation_unit& tu, cpp_cursor cur,
                                              const cpp_entity& parent,
                                              unsigned          template_offset = 0);

        cpp_name get_name() const override;

        cpp_name get_signature() const override
        {
            return signature_;
        }

    private:
        cpp_constructor(cpp_cursor cur, const cpp_entity& parent, cpp_function_info info);

        std::string signature_;

        friend detail::cpp_ptr_access;
    };

    class cpp_destructor final : public cpp_function_base
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::destructor_t;
        }

        static cpp_ptr<cpp_destructor> parse(translation_unit& tu, cpp_cursor cur,
                                             const cpp_entity& parent,
                                             unsigned          template_offset = 0);

        cpp_name get_name() const override;

        cpp_virtual get_virtual() const STANDARDESE_NOEXCEPT
        {
            return virtual_;
        }

        cpp_name get_signature() const override
        {
            return "()";
        }

    private:
        cpp_destructor(cpp_cursor cur, const cpp_entity& parent, cpp_function_info info,
                       cpp_virtual virt)
        : cpp_function_base(get_entity_type(), cur, parent, std::move(info)), virtual_(virt)
        {
        }

        cpp_virtual virtual_;

        friend detail::cpp_ptr_access;
    };

    namespace detail
    {
        inline bool is_virtual(const cpp_entity& e)
        {
            if (e.get_entity_type() == cpp_entity::member_function_t)
                return standardese::is_virtual(
                    static_cast<const cpp_member_function&>(e).get_virtual());
            else if (e.get_entity_type() == cpp_entity::conversion_op_t)
                return standardese::is_virtual(
                    static_cast<const cpp_conversion_op&>(e).get_virtual());
            else if (e.get_entity_type() == cpp_entity::destructor_t)
                return standardese::is_virtual(static_cast<const cpp_destructor&>(e).get_virtual());

            return false;
        }
    } // namespace detail
} // namespace standardese

#endif // STANDARDESE_CPP_FUNCTION_HPP_INCLUDED
