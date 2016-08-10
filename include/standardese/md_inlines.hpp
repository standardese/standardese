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

        static md_ptr<md_text> parse(cmark_node* cur, const md_entity& parent);

        static md_ptr<md_text> make(const md_entity& parent, const char* text);

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

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

        static md_ptr<md_soft_break> parse(cmark_node* cur, const md_entity& parent);

        static md_ptr<md_soft_break> make(const md_entity& parent);

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

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

        static md_ptr<md_line_break> parse(cmark_node* cur, const md_entity& parent);

        static md_ptr<md_line_break> make(const md_entity& parent);

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

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

        static md_ptr<md_code> parse(cmark_node* cur, const md_entity& parent);

        static md_ptr<md_code> make(const md_entity& parent, const char* code);

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

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

        static md_ptr<md_emphasis> parse(cmark_node* cur, const md_entity& parent);

        static md_ptr<md_emphasis> make(const md_entity& parent);

        static md_ptr<md_emphasis> make(const md_entity& parent, const char* str);

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

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

        static md_ptr<md_strong> parse(cmark_node* cur, const md_entity& parent);

        static md_ptr<md_strong> make(const md_entity& parent);

        static md_ptr<md_strong> make(const md_entity& parent, const char* str);

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

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

        static md_ptr<md_link> parse(cmark_node* cur, const md_entity& parent);

        static md_ptr<md_link> make(const md_entity& parent, const char* destination,
                                    const char* title);

        const char* get_title() const STANDARDESE_NOEXCEPT;

        const char* get_destination() const STANDARDESE_NOEXCEPT;

        void set_destination(const char* dest);

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

    private:
        md_link(cmark_node* cur, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_container(get_entity_type(), cur, parent)
        {
        }

        friend detail::md_ptr_access;
    };

    class md_anchor final : public md_leave
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::anchor_t;
        }

        static md_ptr<md_anchor> make(const md_entity& parent, const char* id);

        std::string get_id() const;

        void set_id(const char* id);

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

    private:
        md_anchor(cmark_node* cur, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_leave(get_entity_type(), cur, parent)
        {
        }

        friend detail::md_ptr_access;
    };
} // namespace standardese

#endif // STANDARDESE_MD_INLINES_HPP_INCLUDED
