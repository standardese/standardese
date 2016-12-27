// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MD_ENTITY_HPP_INCLUDED
#define STANDARDESE_MD_ENTITY_HPP_INCLUDED

#include <memory>

#include <standardese/detail/entity_container.hpp>
#include <standardese/string.hpp>

extern "C" {
typedef struct cmark_node cmark_node;
}

namespace standardese
{
    template <typename T>
    using md_ptr = std::unique_ptr<T>;

    class md_entity;

    using md_entity_ptr = md_ptr<md_entity>;

    class md_entity
    {
    public:
        enum type
        {
            _begin_block,

            document_t = _begin_block,
            comment_t,
            block_quote_t,
            list_t,
            list_item_t,
            code_block_t,
            paragraph_t,
            heading_t,
            thematic_break_t,

            inline_documentation_t,
            code_block_advanced_t,

            _end_block,
            _begin_inline = _end_block,

            text_t = _begin_inline,
            soft_break_t,
            line_break_t,
            code_t,
            emphasis_t,
            strong_t,
            link_t,
            anchor_t,

            section_t,

            _end_inline,
            count = _end_inline,
        };

        static md_entity_ptr try_parse(cmark_node* cur, const md_entity& parent);

        md_entity(md_entity&&) = delete;

        md_entity(const md_entity&) = delete;

        virtual ~md_entity() STANDARDESE_NOEXCEPT;

        md_entity& operator=(md_entity&&) = delete;

        md_entity& operator=(const md_entity&) = delete;

        type get_entity_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

        cmark_node* get_node() const STANDARDESE_NOEXCEPT
        {
            // screw const-correctness
            return node_;
        }

        bool has_parent() const STANDARDESE_NOEXCEPT
        {
            return parent_ != nullptr;
        }

        const md_entity& get_parent() const STANDARDESE_NOEXCEPT
        {
            return *parent_;
        }

        md_entity_ptr clone(const md_entity& parent) const
        {
            return do_clone(&parent);
        }

    protected:
        md_entity(type t, cmark_node* node, const md_entity& parent) STANDARDESE_NOEXCEPT
            : parent_(&parent),
              node_(node),
              type_(t)
        {
        }

        md_entity(type t, cmark_node* node) STANDARDESE_NOEXCEPT : parent_(nullptr),
                                                                   node_(node),
                                                                   type_(t)
        {
        }

        virtual md_entity_ptr do_clone(const md_entity* parent) const = 0;

    private:
        md_ptr<md_entity> next_;
        const md_entity*  parent_;
        cmark_node*       node_;
        type              type_;

        template <class T, class Base, template <typename> class Ptr>
        friend class detail::entity_container;

        friend class md_container;
    };

    inline bool is_block(md_entity::type t) STANDARDESE_NOEXCEPT
    {
        return t >= md_entity::_begin_block && t < md_entity::_end_block;
    }

    inline bool is_inline(md_entity::type t) STANDARDESE_NOEXCEPT
    {
        return t >= md_entity::_begin_inline && t < md_entity::_end_inline;
    }

    inline bool is_leave(md_entity::type t) STANDARDESE_NOEXCEPT
    {
        return t == md_entity::thematic_break_t || t == md_entity::code_block_t
               || t == md_entity::text_t || t == md_entity::soft_break_t
               || t == md_entity::line_break_t || t == md_entity::code_t || t == md_entity::anchor_t
               || t == md_entity::code_block_advanced_t;
    }

    inline bool is_container(md_entity::type t) STANDARDESE_NOEXCEPT
    {
        return !is_leave(t);
    }

    using md_entity_container = detail::entity_container<md_entity, md_entity, md_ptr>;

    class md_leave : public md_entity
    {
    public:
        const char* get_string() const STANDARDESE_NOEXCEPT;

        void set_string(const char* str);

    protected:
        md_leave(md_entity::type t, cmark_node* node, const md_entity& parent) STANDARDESE_NOEXCEPT;
    };

    class md_container : public md_entity, public md_entity_container
    {
    public:
        virtual md_entity& add_entity(md_entity_ptr entity);

        md_entity& front() STANDARDESE_NOEXCEPT
        {
            return *begin();
        }

        const md_entity& front() const STANDARDESE_NOEXCEPT
        {
            return *begin();
        }

        md_entity& back() STANDARDESE_NOEXCEPT
        {
            return *get_last();
        }

        const md_entity& back() const STANDARDESE_NOEXCEPT
        {
            return *get_last();
        }

    protected:
        md_container(md_entity::type t, cmark_node* node,
                     const md_entity& parent) STANDARDESE_NOEXCEPT;

        md_container(md_entity::type t, cmark_node* node) STANDARDESE_NOEXCEPT;
    };

    namespace detail
    {
        struct md_ptr_access
        {
            template <typename T, typename... Args>
            static md_ptr<T> make(Args&&... args)
            {
                return md_ptr<T>(new T(std::forward<Args>(args)...));
            }
        };

        template <typename T, typename... Args>
        md_ptr<T> make_md_ptr(Args&&... args)
        {
            return md_ptr_access::make<T>(std::forward<Args>(args)...);
        }
    } // namespace detail
} // namespace standardese

#endif // STANDARDESE_MD_ENTITY_HPP_INCLUDED
