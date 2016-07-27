// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_TEMPLATE_HPP_INCLUDED
#define STANDARDESE_CPP_TEMPLATE_HPP_INCLUDED

#include <standardese/cpp_class.hpp>
#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_entity_registry.hpp>
#include <standardese/cpp_type.hpp>

namespace standardese
{
    using cpp_template_ref = basic_cpp_entity_ref<CXCursor_TemplateRef>;

    class cpp_template_parameter : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_template_parameter> try_parse(translation_unit& tu, cpp_cursor cur,
                                                         const cpp_entity& parent);

        bool is_variadic() const STANDARDESE_NOEXCEPT
        {
            return variadic_;
        }

    protected:
        cpp_template_parameter(cpp_entity::type t, cpp_cursor cur, const cpp_entity& e,
                               bool is_variadic)
        : cpp_entity(t, cur, e), variadic_(is_variadic)
        {
        }

    private:
        cpp_name do_get_unique_name() const override;

        bool variadic_;
    };

    class cpp_template_type_parameter final : public cpp_template_parameter
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::template_type_parameter_t;
        }

        static cpp_ptr<cpp_template_type_parameter> parse(translation_unit& tu, cpp_cursor cur,
                                                          const cpp_entity& parent);

        bool has_default_type() const STANDARDESE_NOEXCEPT
        {
            return !default_.get_name().empty();
        }

        const cpp_type_ref& get_default_type() const STANDARDESE_NOEXCEPT
        {
            return default_;
        }

    private:
        cpp_template_type_parameter(cpp_cursor cur, const cpp_entity& parent, cpp_type_ref def,
                                    bool is_variadic)
        : cpp_template_parameter(get_entity_type(), cur, parent, is_variadic),
          default_(std::move(def))
        {
        }

        cpp_type_ref default_;

        friend detail::cpp_ptr_access;
    };

    class cpp_non_type_template_parameter final : public cpp_template_parameter
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::non_type_template_parameter_t;
        }

        static cpp_ptr<cpp_non_type_template_parameter> parse(translation_unit& tu, cpp_cursor cur,
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
        cpp_non_type_template_parameter(cpp_cursor cur, const cpp_entity& parent, cpp_type_ref type,
                                        std::string def, bool is_variadic)
        : cpp_template_parameter(get_entity_type(), cur, parent, is_variadic),
          type_(std::move(type)),
          default_(std::move(def))
        {
        }

        cpp_type_ref type_;
        std::string  default_;

        friend detail::cpp_ptr_access;
    };

    class cpp_template_template_parameter final
        : public cpp_template_parameter,
          public cpp_entity_container<cpp_template_parameter>
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::template_template_parameter_t;
        }

        static cpp_ptr<cpp_template_template_parameter> parse(translation_unit& tu, cpp_cursor cur,
                                                              const cpp_entity& parent);

        void add_paramter(cpp_ptr<cpp_template_parameter> param)
        {
            cpp_entity_container<cpp_template_parameter>::add_entity(std::move(param));
        }

        bool has_default_template() const STANDARDESE_NOEXCEPT
        {
            return !default_.get_name().empty();
        }

        const cpp_template_ref& get_default_template() const STANDARDESE_NOEXCEPT
        {
            return default_;
        }

    private:
        cpp_template_template_parameter(cpp_cursor cur, const cpp_entity& parent,
                                        cpp_template_ref def, bool is_variadic)
        : cpp_template_parameter(get_entity_type(), cur, parent, is_variadic),
          default_(std::move(def))
        {
        }

        cpp_template_ref default_;

        friend detail::cpp_ptr_access;
    };

    /// Returns whether cur refers to a full specialization of a template.
    /// cur must refer to a class or function.
    bool is_full_specialization(translation_unit& tu, cpp_cursor cur);

    class cpp_function_base;

    class cpp_function_template final : public cpp_entity,
                                        private cpp_entity_container<cpp_template_parameter>
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::function_template_t;
        }

        static cpp_ptr<cpp_function_template> parse(translation_unit& tu, cpp_cursor cur,
                                                    const cpp_entity& parent);

        void add_template_parameter(cpp_ptr<cpp_template_parameter> param)
        {
            cpp_entity_container<cpp_template_parameter>::add_entity(std::move(param));
        }

        const cpp_entity_container<cpp_template_parameter>& get_template_parameters() const
            STANDARDESE_NOEXCEPT
        {
            return *this;
        }

        cpp_name get_name() const override;

        cpp_name get_signature() const;

        const cpp_function_base& get_function() const STANDARDESE_NOEXCEPT
        {
            return *func_;
        }

    private:
        cpp_name do_get_unique_name() const override;

        cpp_function_template(cpp_cursor cur, const cpp_entity& parent);

        cpp_ptr<cpp_function_base> func_;

        friend detail::cpp_ptr_access;
    };

    class cpp_function_template_specialization final : public cpp_entity
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::function_template_specialization_t;
        }

        static cpp_ptr<cpp_function_template_specialization> parse(translation_unit& tu,
                                                                   cpp_cursor        cur,
                                                                   const cpp_entity& parent);

        cpp_name get_name() const override
        {
            return name_;
        }

        cpp_name get_signature() const;

        const cpp_function_base& get_function() const STANDARDESE_NOEXCEPT
        {
            return *func_;
        }

        const cpp_template_ref& get_primary_template() const STANDARDESE_NOEXCEPT
        {
            return primary_;
        }

    private:
        cpp_name do_get_unique_name() const override;

        cpp_function_template_specialization(cpp_cursor cur, const cpp_entity& parent);

        cpp_template_ref           primary_;
        cpp_name                   name_;
        cpp_ptr<cpp_function_base> func_;

        friend detail::cpp_ptr_access;
        friend cpp_function_base;
    };

    /// \returns If `e` is a function type returns a pointer to `e`,
    /// otherwise if `e` is a function template, returns `&e.get_function()`,
    /// otherwise returns `nullptr`.
    const cpp_function_base* get_function(const cpp_entity& e) STANDARDESE_NOEXCEPT;

    class cpp_class_template final : public cpp_entity,
                                     private cpp_entity_container<cpp_template_parameter>
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::class_template_t;
        }

        static cpp_ptr<cpp_class_template> parse(translation_unit& tu, cpp_cursor cur,
                                                 const cpp_entity& parent);

        void add_template_parameter(cpp_ptr<cpp_template_parameter> param)
        {
            cpp_entity_container<cpp_template_parameter>::add_entity(std::move(param));
        }

        void add_entity(cpp_entity_ptr ptr)
        {
            class_->add_entity(std::move(ptr));
        }

        cpp_name get_name() const override;

        const cpp_entity_container<cpp_template_parameter>& get_template_parameters() const
            STANDARDESE_NOEXCEPT
        {
            return *this;
        }

        const cpp_class& get_class() const STANDARDESE_NOEXCEPT
        {
            return *class_;
        }

    private:
        cpp_class_template(cpp_cursor cur, const cpp_entity& parent)
        : cpp_entity(get_entity_type(), cur, parent)
        {
        }

        cpp_ptr<cpp_class> class_;

        friend detail::cpp_ptr_access;
    };

    class cpp_class_template_full_specialization final : public cpp_entity
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::class_template_full_specialization_t;
        }

        static cpp_ptr<cpp_class_template_full_specialization> parse(translation_unit& tu,
                                                                     cpp_cursor        cur,
                                                                     const cpp_entity& parent);

        void add_entity(cpp_entity_ptr ptr)
        {
            class_->add_entity(std::move(ptr));
        }

        cpp_name get_name() const override
        {
            return name_;
        }

        const cpp_class& get_class() const STANDARDESE_NOEXCEPT
        {
            return *class_;
        }

        const cpp_template_ref& get_primary_template() const STANDARDESE_NOEXCEPT
        {
            return primary_;
        }

    private:
        cpp_class_template_full_specialization(cpp_cursor cur, const cpp_entity& parent)
        : cpp_entity(get_entity_type(), cur, parent), name_("")
        {
        }

        cpp_template_ref   primary_;
        cpp_name           name_;
        cpp_ptr<cpp_class> class_;

        friend detail::cpp_ptr_access;
        friend cpp_class;
    };

    class cpp_class_template_partial_specialization final
        : public cpp_entity,
          public cpp_entity_container<cpp_template_parameter>
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::class_template_partial_specialization_t;
        }

        static cpp_ptr<cpp_class_template_partial_specialization> parse(translation_unit& tu,
                                                                        cpp_cursor        cur,
                                                                        const cpp_entity& parent);

        void add_template_parameter(cpp_ptr<cpp_template_parameter> param)
        {
            cpp_entity_container<cpp_template_parameter>::add_entity(std::move(param));
        }

        void add_entity(cpp_entity_ptr ptr)
        {
            class_->add_entity(std::move(ptr));
        }

        cpp_name get_name() const override
        {
            return name_;
        }

        const cpp_entity_container<cpp_template_parameter>& get_template_parameters() const
            STANDARDESE_NOEXCEPT
        {
            return *this;
        }

        const cpp_class& get_class() const STANDARDESE_NOEXCEPT
        {
            return *class_;
        }

        const cpp_template_ref& get_primary_template() const STANDARDESE_NOEXCEPT
        {
            return primary_;
        }

    private:
        cpp_class_template_partial_specialization(cpp_cursor cur, const cpp_entity& parent)
        : cpp_entity(get_entity_type(), cur, parent), name_("")
        {
        }

        cpp_template_ref   primary_;
        cpp_name           name_;
        cpp_ptr<cpp_class> class_;

        friend detail::cpp_ptr_access;
        friend cpp_class;
    };

    /// \returns If `e` is a class returns a pointer to `e`,
    /// otherwise if `e` is a class template returns `&e.get_class()`,
    /// otherwise returns `nullptr`.
    const cpp_class* get_class(const cpp_entity& e) STANDARDESE_NOEXCEPT;

    class cpp_alias_template final : public cpp_entity,
                                     private cpp_entity_container<cpp_template_parameter>
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::alias_template_t;
        }

#if CINDEX_VERSION_MINOR >= 32
        static cpp_ptr<cpp_alias_template> parse(translation_unit& tu, cpp_cursor cur,
                                                 const cpp_entity& parent);
#endif

        void add_template_parameter(cpp_ptr<cpp_template_parameter> param)
        {
            cpp_entity_container<cpp_template_parameter>::add_entity(std::move(param));
        }

        cpp_name get_name() const override;

        const cpp_entity_container<cpp_template_parameter>& get_template_parameters() const
            STANDARDESE_NOEXCEPT
        {
            return *this;
        }

        const cpp_type_alias& get_type() const STANDARDESE_NOEXCEPT
        {
            return *type_;
        }

    private:
        cpp_alias_template(cpp_cursor cur, const cpp_entity& parent)
        : cpp_entity(get_entity_type(), cur, parent)
        {
        }

        cpp_ptr<cpp_type_alias> type_;

        friend detail::cpp_ptr_access;
    };

    const cpp_entity_container<cpp_template_parameter>* get_template_parameters(
        const cpp_entity& e);
} // namespace standardese

#endif // STANDARDESE_CPP_TEMPLATE_HPP_INCLUDED
