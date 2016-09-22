// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_ENTITY_HPP_INCLUDED
#define STANDARDESE_CPP_ENTITY_HPP_INCLUDED

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

#include <standardese/detail/entity_container.hpp>
#include <standardese/cpp_cursor.hpp>
#include <standardese/noexcept.hpp>
#include <standardese/string.hpp>

namespace standardese
{
    class translation_unit;
    class parser;

    using cpp_name = string;

    template <typename T>
    using cpp_ptr = std::unique_ptr<T>;

    class cpp_entity;

    using cpp_entity_ptr = cpp_ptr<cpp_entity>;

    class cpp_entity
    {
    public:
        enum type
        {
            file_t,

            inclusion_directive_t,
            macro_definition_t,

            language_linkage_t,

            namespace_t,
            namespace_alias_t,
            using_directive_t,
            using_declaration_t,

            type_alias_t,
            alias_template_t,

            enum_t,
            signed_enum_value_t,
            unsigned_enum_value_t,
            expression_enum_value_t,

            variable_t,
            member_variable_t,
            bitfield_t,

            function_parameter_t,
            function_t,
            member_function_t,
            conversion_op_t,
            constructor_t,
            destructor_t,

            template_type_parameter_t,
            non_type_template_parameter_t,
            template_template_parameter_t,

            function_template_t,
            function_template_specialization_t,

            class_t,
            class_template_t,
            class_template_partial_specialization_t,
            class_template_full_specialization_t,

            base_class_t,
            access_specifier_t,

            invalid_t
        };

        static cpp_entity_ptr try_parse(translation_unit& tu, cpp_cursor cur,
                                        const cpp_entity& ast_parent);

        cpp_entity(cpp_entity&&) = delete;

        cpp_entity(const cpp_entity&) = delete;

        virtual ~cpp_entity() STANDARDESE_NOEXCEPT = default;

        cpp_entity& operator=(const cpp_entity&) = delete;

        cpp_entity& operator=(cpp_entity&&) = delete;

        /// \returns The name of the entity as specified in the source.
        virtual cpp_name get_name() const;

        virtual cpp_name get_scope() const;

        /// \returns The full name of the entity, scope followed by name.
        cpp_name get_full_name() const
        {
            auto scope = get_scope();
            return scope.empty() ? get_name() :
                                   std::string(scope.c_str()) + "::" + get_name().c_str();
        }

        /// \returns A unique name describing one entity.
        cpp_name get_unique_name() const
        {
            return do_get_unique_name();
        }

        /// \returns The type of the entity.
        type get_entity_type() const STANDARDESE_NOEXCEPT
        {
            return t_;
        }

        /// \returns The libclang cursor of the declaration of the entity.
        cpp_cursor get_cursor() const STANDARDESE_NOEXCEPT
        {
            return cursor_;
        }

        bool has_ast_parent() const STANDARDESE_NOEXCEPT
        {
            return parent_ != nullptr;
        }

        /// \returns The AST parent entity,
        /// that is the entity which owns the current one.
        /// \requires The entity has a parent.
        const cpp_entity& get_ast_parent() const STANDARDESE_NOEXCEPT
        {
            return *parent_;
        }

        const cpp_entity* get_semantic_parent() const STANDARDESE_NOEXCEPT
        {
            auto cur = parent_;
            while (cur && !cur->is_semantic_parent())
                cur = cur->parent_;
            return cur;
        }

    protected:
        cpp_entity(type t, cpp_cursor cur, const cpp_entity& ast_parent);

        cpp_entity(type t, cpp_cursor cur);

        void set_cursor(cpp_cursor cur) STANDARDESE_NOEXCEPT
        {
            cursor_ = cur;
        }

    private:
        virtual cpp_name do_get_unique_name() const
        {
            return get_full_name();
        }

        virtual bool is_semantic_parent() const STANDARDESE_NOEXCEPT
        {
            return true;
        }

        cpp_cursor        cursor_;
        cpp_entity_ptr    next_;
        const cpp_entity* parent_;
        type              t_;

        template <typename T, class Base, template <typename> class Ptr>
        friend class detail::entity_container;
        template <typename T>
        friend class cpp_entity_container;
    };

    inline bool is_preprocessor(cpp_entity::type t) STANDARDESE_NOEXCEPT
    {
        return t == cpp_entity::inclusion_directive_t || t == cpp_entity::macro_definition_t;
    }

    inline bool is_variable(cpp_entity::type t) STANDARDESE_NOEXCEPT
    {
        return t == cpp_entity::variable_t || t == cpp_entity::member_variable_t
               || t == cpp_entity::bitfield_t;
    }

    inline bool is_type(cpp_entity::type t) STANDARDESE_NOEXCEPT
    {
        return t == cpp_entity::enum_t || t == cpp_entity::class_t || t == cpp_entity::type_alias_t;
    }

    inline bool is_type_template(cpp_entity::type t) STANDARDESE_NOEXCEPT
    {
        return t == cpp_entity::class_template_t
               || t == cpp_entity::class_template_partial_specialization_t
               || t == cpp_entity::class_template_full_specialization_t
               || t == cpp_entity::alias_template_t;
    }

    inline bool is_function_like(cpp_entity::type t) STANDARDESE_NOEXCEPT
    {
        return t == cpp_entity::function_t || t == cpp_entity::member_function_t
               || t == cpp_entity::conversion_op_t || t == cpp_entity::constructor_t
               || t == cpp_entity::destructor_t;
    }

    inline bool is_function_template(cpp_entity::type t) STANDARDESE_NOEXCEPT
    {
        return t == cpp_entity::function_template_t
               || t == cpp_entity::function_template_specialization_t;
    }

    inline bool is_template(cpp_entity::type t) STANDARDESE_NOEXCEPT
    {
        return is_function_template(t) || is_type_template(t);
    }

    inline bool is_template_parameter(cpp_entity::type t) STANDARDESE_NOEXCEPT
    {
        return t == cpp_entity::template_type_parameter_t
               || t == cpp_entity::non_type_template_parameter_t
               || t == cpp_entity::template_template_parameter_t;
    }

    inline bool is_parameter(cpp_entity::type t) STANDARDESE_NOEXCEPT
    {
        return t == cpp_entity::function_parameter_t || is_template_parameter(t);
    }

    inline bool is_member_variable(cpp_entity::type t) STANDARDESE_NOEXCEPT
    {
        return t == cpp_entity::member_variable_t || t == cpp_entity::bitfield_t;
    }

    template <class T>
    class cpp_entity_container : public detail::entity_container<T, cpp_entity, cpp_ptr>
    {
    protected:
        cpp_entity_container() = default;

        void add_entity(const cpp_entity* this_entity, cpp_entity_ptr entity)
        {
            if (this_entity
                && (!entity->has_ast_parent() || &entity->get_ast_parent() != this_entity))
                entity->parent_ = this_entity;
            detail::entity_container<T, cpp_entity, cpp_ptr>::add_entity(std::move(entity));
        }

        cpp_entity_ptr remove_entity_after(cpp_entity* base)
        {
            auto entity =
                detail::entity_container<T, cpp_entity, cpp_ptr>::remove_entity_after(base);
            entity->parent_ = nullptr;
            return std::move(entity);
        }
    };

    namespace detail
    {
        struct cpp_ptr_access
        {
            template <typename T, typename... Args>
            static cpp_ptr<T> make(Args&&... args)
            {
                return cpp_ptr<T>(new T(std::forward<Args>(args)...));
            }
        };

        template <typename T, typename... Args>
        cpp_ptr<T> make_cpp_ptr(Args&&... args)
        {
            return cpp_ptr_access::make<T>(std::forward<Args>(args)...);
        }

        template <typename Derived>
        cpp_ptr<Derived> downcast(cpp_entity_ptr entity)
        {
            auto ptr = entity.release();
            return cpp_ptr<Derived>(static_cast<Derived*>(ptr));
        }
    } // namespace detail
} // namespace standardese

#endif // STANDARDESE_CPP_ENTITY_HPP_INCLUDED
