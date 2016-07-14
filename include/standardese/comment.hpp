// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_HPP_INCLUDED
#define STANDARDESE_COMMENT_HPP_INCLUDED

#include <string>
#include <vector>

#include <standardese/md_entity.hpp>

namespace standardese
{
    class parser;

    class md_comment final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::comment_t;
        }

        static md_ptr<md_comment> parse(const parser& p, const string& name, const string& comment);

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
        md_comment(std::string id);

        std::string id_;
        bool        excluded_;

        friend detail::md_ptr_access;
    };
} // namespace standardese

#endif // STANDARDESE_COMMENT_HPP_INCLUDED
