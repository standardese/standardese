// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_HPP_INCLUDED
#define STANDARDESE_COMMENT_HPP_INCLUDED

#include <string>
#include <vector>

#include <standardese/md_entity.hpp>
#include <standardese/md_blocks.hpp>

namespace standardese
{
    class parser;

    namespace detail
    {
        struct raw_comment
        {
            std::string content;
            unsigned    count_lines, end_line;

            raw_comment(std::string content, unsigned count_lines, unsigned end_line)
            : content(std::move(content)), count_lines(count_lines), end_line(end_line)
            {
            }
        };

        std::vector<raw_comment> read_comments(const std::string& source);
    } // namespace detail

    class md_comment final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::comment_t;
        }

        static md_ptr<md_comment> parse(const parser& p, const string& name, const string& comment);

        md_entity& add_entity(md_entity_ptr ptr) override;

        md_paragraph& get_brief() STANDARDESE_NOEXCEPT
        {
            assert(begin()->get_entity_type() == md_entity::paragraph_t);
            auto& brief = static_cast<md_paragraph&>(*begin());
            assert(brief.get_section_type() == section_type::brief);
            return brief;
        }

        const md_paragraph& get_brief() const STANDARDESE_NOEXCEPT
        {
            assert(begin()->get_entity_type() == md_entity::paragraph_t);
            auto& brief = static_cast<const md_paragraph&>(*begin());
            assert(brief.get_section_type() == section_type::brief);
            return brief;
        }

        bool is_excluded() const STANDARDESE_NOEXCEPT
        {
            return excluded_;
        }

        void set_excluded(bool b) STANDARDESE_NOEXCEPT
        {
            excluded_ = b;
        }

        md_entity_ptr clone() const
        {
            return do_clone(nullptr);
        }

        std::string get_output_name() const;

        bool has_unique_name() const STANDARDESE_NOEXCEPT
        {
            return !id_.empty();
        }

        const std::string& get_unique_name() const STANDARDESE_NOEXCEPT
        {
            return id_;
        }

        void set_unique_name(std::string id)
        {
            id_ = std::move(id);
        }

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

    private:
        md_comment();

        std::string id_;
        bool        excluded_;

        friend detail::md_ptr_access;
    };
} // namespace standardese

#endif // STANDARDESE_COMMENT_HPP_INCLUDED
