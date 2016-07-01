// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MD_BLOCKS_HPP_INCLUDED
#define STANDARDESE_MD_BLOCKS_HPP_INCLUDED

#include <standardese/md_entity.hpp>
#include <standardese/section.hpp>

namespace standardese
{
    class md_block_quote final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::block_quote_t;
        }

        static md_ptr<md_block_quote> parse(comment& c, cmark_node* cur, const md_entity& parent);

        static md_ptr<md_block_quote> make(const md_entity& parent);

    private:
        md_block_quote(cmark_node* node, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_container(get_entity_type(), node, parent)
        {
        }

        friend detail::md_ptr_access;
    };

    enum class md_list_type
    {
        bullet,
        ordered,
    };

    /// The delimiter of an ordered list, i.e. "1." vs "1)".
    enum class md_list_delimiter
    {
        none,
        period,
        parenthesis,
    };

    class md_list final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::list_t;
        }

        static md_ptr<md_list> parse(comment& c, cmark_node* cur, const md_entity& parent);

        static md_ptr<md_list> make(const md_entity& parent, md_list_type type,
                                    md_list_delimiter delim, int start, bool is_tight);

        md_list_type get_list_type() const STANDARDESE_NOEXCEPT;

        md_list_delimiter get_delimiter() const STANDARDESE_NOEXCEPT;

        /// \returns The starting number of the ordered list.
        int get_start() const STANDARDESE_NOEXCEPT;

        /// \returns Whether or not the list is tight.
        /// A tight list isn't seperated by blank lines.
        bool is_tight() const STANDARDESE_NOEXCEPT;

    private:
        md_list(cmark_node* node, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_container(get_entity_type(), node, parent)
        {
        }

        friend detail::md_ptr_access;
    };

    class md_list_item final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::list_item_t;
        }

        static md_ptr<md_list_item> parse(comment& c, cmark_node* cur, const md_entity& parent);

        static md_ptr<md_list_item> make(const md_entity& parent);

    private:
        md_list_item(cmark_node* node, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_container(get_entity_type(), node, parent)
        {
        }

        friend detail::md_ptr_access;
    };

    class md_code_block final : public md_leave
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::code_block_t;
        }

        static md_ptr<md_code_block> parse(comment& c, cmark_node* cur, const md_entity& parent);

        static md_ptr<md_code_block> make(const md_entity& parent, const char* code,
                                          const char* fence);

        const char* get_fence_info() const STANDARDESE_NOEXCEPT;

    private:
        md_code_block(cmark_node* node, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_leave(get_entity_type(), node, parent)
        {
        }

        friend detail::md_ptr_access;
    };

    class md_paragraph final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::paragraph_t;
        }

        static md_ptr<md_paragraph> parse(comment& c, cmark_node* cur, const md_entity& parent);

        static md_ptr<md_paragraph> make(const md_entity& parent);

        section_type get_section_type() const STANDARDESE_NOEXCEPT
        {
            return section_type_;
        }

        void set_section_type(section_type t) STANDARDESE_NOEXCEPT
        {
            section_type_ = t;
        }

    private:
        md_paragraph(cmark_node* node, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_container(get_entity_type(), node, parent),
              section_type_(section_type::details)
        {
        }

        section_type section_type_;

        friend detail::md_ptr_access;
    };

    class md_heading final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::heading_t;
        }

        static md_ptr<md_heading> parse(comment& c, cmark_node* cur, const md_entity& parent);

        static md_ptr<md_heading> make(const md_entity& parent, int level);

        int get_level() const STANDARDESE_NOEXCEPT;

    private:
        md_heading(cmark_node* node, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_container(get_entity_type(), node, parent)
        {
        }

        friend detail::md_ptr_access;
    };

    class md_thematic_break final : public md_leave
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::thematic_break_t;
        }

        static md_ptr<md_thematic_break> parse(comment& c, cmark_node* cur,
                                               const md_entity& parent);

        static md_ptr<md_thematic_break> make(const md_entity& parent);

    private:
        md_thematic_break(cmark_node* node, const md_entity& parent) STANDARDESE_NOEXCEPT
            : md_leave(get_entity_type(), node, parent)
        {
        }

        friend detail::md_ptr_access;
    };
} // namespace standardese

#endif // STANDARDESE_MD_BLOCKS_HPP_INCLUDED
