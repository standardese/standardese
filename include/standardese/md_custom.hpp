// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MD_CUSTOM_HPP_INCLUDED
#define STANDARDESE_MD_CUSTOM_HPP_INCLUDED

#include <standardese/md_entity.hpp>

namespace standardese
{
    class md_section final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::section_t;
        }

        static md_ptr<md_section> make(const md_entity& parent, const std::string& section_text);

        const char* get_section_text() const STANDARDESE_NOEXCEPT;

        void set_section_text(const std::string& text);

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

    private:
        md_section(const md_entity& parent, const std::string& section_text);

        friend detail::md_ptr_access;
    };

    class md_inline_documentation final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::inline_documentation_t;
        }

        static md_ptr<md_inline_documentation> make(const md_entity&   parent,
                                                    const std::string& heading);

        // adds the contents of all paragraphs in container
        // returns false if any child was not a paragraph
        bool add_item(const char* name, const char* id, const md_container& container);

        bool empty() const STANDARDESE_NOEXCEPT;

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

    private:
        md_inline_documentation(const md_entity& parent);

        friend detail::md_ptr_access;
    };

    class md_document final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::document_t;
        }

        static md_ptr<md_document> make(std::string name);

        md_entity_ptr clone() const
        {
            return do_clone(nullptr);
        }

        const std::string& get_output_name() const STANDARDESE_NOEXCEPT
        {
            return name_;
        }

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

    private:
        md_document(cmark_node* node, std::string name)
        : md_container(get_entity_type(), node), name_(std::move(name))
        {
        }

        std::string name_;

        friend detail::md_ptr_access;
    };
} // namespace standardese

#endif // STANDARDESE_MD_CUSTOM_HPP_INCLUDED
