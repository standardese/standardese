// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MD_INLINES_HPP_INCLUDED
#define STANDARDESE_MD_INLINES_HPP_INCLUDED

#include <standardese/md_entity.hpp>

namespace standardese
{
    class md_text final : public md_leave
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::text_t;
        }

        static md_ptr<md_text> parse(comment& c, cmark_node* cur, const md_entity& parent);

    private:
        md_text(cmark_node* cur, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_leave(get_entity_type(), cur, parent)
        {
        }

        friend detail::md_ptr_access;
    };

    class md_soft_break final : public md_leave
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::soft_break_t;
        }

        static md_ptr<md_soft_break> parse(comment& c, cmark_node* cur, const md_entity& parent);

    private:
        md_soft_break(cmark_node* cur, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_leave(get_entity_type(), cur, parent)
        {
        }

        friend detail::md_ptr_access;
    };

    class md_line_break final : public md_leave
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::line_break_t;
        }

        static md_ptr<md_line_break> parse(comment& c, cmark_node* cur, const md_entity& parent);

    private:
        md_line_break(cmark_node* cur, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_leave(get_entity_type(), cur, parent)
        {
        }

        friend detail::md_ptr_access;
    };

    class md_code final : public md_leave
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::code_t;
        }

        static md_ptr<md_code> parse(comment& c, cmark_node* cur, const md_entity& parent);

    private:
        md_code(cmark_node* cur, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_leave(get_entity_type(), cur, parent)
        {
        }

        friend detail::md_ptr_access;
    };

    class md_emphasis final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::emphasis_t;
        }

        static md_ptr<md_emphasis> parse(comment& c, cmark_node* cur, const md_entity& parent);

    private:
        md_emphasis(cmark_node* cur, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_container(get_entity_type(), cur, parent)
        {
        }

        friend detail::md_ptr_access;
    };

    class md_strong final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::strong_t;
        }

        static md_ptr<md_strong> parse(comment& c, cmark_node* cur, const md_entity& parent);

    private:
        md_strong(cmark_node* cur, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_container(get_entity_type(), cur, parent)
        {
        }

        friend detail::md_ptr_access;
    };

    class md_link final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::link_t;
        }

        static md_ptr<md_link> parse(comment& c, cmark_node* cur, const md_entity& parent);

        const char* get_title() const STANDARDESE_NOEXCEPT;

        const char* get_destination() const STANDARDESE_NOEXCEPT;

    private:
        md_link(cmark_node* cur, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_container(get_entity_type(), cur, parent)
        {
        }

        friend detail::md_ptr_access;
    };
} // namespace standardese

#endif // STANDARDESE_MD_INLINES_HPP_INCLUDED
