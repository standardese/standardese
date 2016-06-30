// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_NAMESPACE_HPP_INCLUDED
#define STANDARDESE_CPP_NAMESPACE_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_entity_registry.hpp>

namespace standardese
{
    class cpp_language_linkage final : public cpp_entity, public cpp_entity_container<cpp_entity>
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::language_linkage_t;
        }

        static cpp_ptr<cpp_language_linkage> parse(translation_unit& tu, cpp_cursor cur,
                                                   const cpp_entity& parent);

        void add_entity(cpp_entity_ptr ptr)
        {
            cpp_entity_container<cpp_entity>::add_entity(std::move(ptr));
        }

        cpp_name get_name() const override
        {
            return name_;
        }

    private:
        cpp_language_linkage(cpp_cursor cur, const cpp_entity& parent, string name)
        : cpp_entity(get_entity_type(), cur, parent), name_(std::move(name))
        {
        }

        string name_;

        friend detail::cpp_ptr_access;
    };

    class cpp_namespace final : public cpp_entity, public cpp_entity_container<cpp_entity>
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::namespace_t;
        }

        static cpp_ptr<cpp_namespace> parse(translation_unit& tu, cpp_cursor cur,
                                            const cpp_entity& parent);

        void add_entity(cpp_entity_ptr ptr)
        {
            cpp_entity_container<cpp_entity>::add_entity(std::move(ptr));
        }

        bool is_inline() const STANDARDESE_NOEXCEPT
        {
            return inline_;
        }

    private:
        cpp_namespace(cpp_cursor cur, const cpp_entity& parent, bool is_inline)
        : cpp_entity(get_entity_type(), cur, parent), inline_(is_inline)
        {
        }

        bool inline_;

        friend detail::cpp_ptr_access;
    };

    using cpp_namespace_ref = basic_cpp_entity_ref<CXCursor_NamespaceRef>;

    class cpp_namespace_alias final : public cpp_entity
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::namespace_alias_t;
        }

        static cpp_ptr<cpp_namespace_alias> parse(translation_unit& tu, cpp_cursor cur,
                                                  const cpp_entity& parent);

        const cpp_namespace_ref& get_target() const STANDARDESE_NOEXCEPT
        {
            return target_;
        }

    private:
        cpp_namespace_alias(cpp_cursor cur, const cpp_entity& parent, cpp_namespace_ref target)
        : cpp_entity(get_entity_type(), cur, parent), target_(target)
        {
        }

        cpp_namespace_ref target_;

        friend detail::cpp_ptr_access;
    };

    class cpp_using_directive final : public cpp_entity
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::using_directive_t;
        }

        static cpp_ptr<cpp_using_directive> parse(translation_unit& tu, cpp_cursor cur,
                                                  const cpp_entity& parent);

        cpp_name get_name() const override
        {
            return "using directive";
        }

        const cpp_namespace_ref& get_target() const STANDARDESE_NOEXCEPT
        {
            return target_;
        }

    private:
        cpp_using_directive(cpp_cursor cur, const cpp_entity& parent, cpp_namespace_ref target)
        : cpp_entity(get_entity_type(), cur, parent), target_(target)
        {
        }

        cpp_namespace_ref target_;

        friend detail::cpp_ptr_access;
    };

    class cpp_using_declaration final : public cpp_entity
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::using_declaration_t;
        }

        static cpp_ptr<cpp_using_declaration> parse(translation_unit& tu, cpp_cursor cur,
                                                    const cpp_entity& parent);

        cpp_name get_name() const override
        {
            return "using declaration";
        }

        const cpp_entity_ref& get_target() const STANDARDESE_NOEXCEPT
        {
            return target_;
        }

    private:
        cpp_using_declaration(cpp_cursor cur, const cpp_entity& parent, cpp_entity_ref target)
        : cpp_entity(get_entity_type(), cur, parent), target_(target)
        {
        }

        cpp_entity_ref target_;

        friend detail::cpp_ptr_access;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_NAMESPACE_HPP_INCLUDED
