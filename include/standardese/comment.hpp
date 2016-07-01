// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_HPP_INCLUDED
#define STANDARDESE_COMMENT_HPP_INCLUDED

#include <string>
#include <vector>

#include <standardese/cpp_entity.hpp>
#include <standardese/md_entity.hpp>
#include <standardese/section.hpp>

namespace standardese
{
    class parser;

    class md_document final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::document_t;
        }

    private:
        md_document(cmark_node* node) : md_container(get_entity_type(), node)
        {
        }

        friend detail::md_ptr_access;
    };

    class comment
    {
    public:
        static comment parse(const parser& p, const cpp_name& name, const cpp_raw_comment& comment);

        const md_document& get_document() const STANDARDESE_NOEXCEPT
        {
            return *document_;
        }

    private:
        comment(md_ptr<md_document> document) : document_(std::move(document))
        {
        }

        md_ptr<md_document> document_;
    };
} // namespace standardese

#endif // STANDARDESE_COMMENT_HPP_INCLUDED
