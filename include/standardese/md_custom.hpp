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

    class md_code_block;

    class md_code_block_advanced final : public md_leave
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::code_block_advanced_t;
        }

        static md_ptr<md_code_block_advanced> make(const md_code_block& cb);
        static md_ptr<md_code_block_advanced> make(const md_entity& parent, const char* code,
                                                   const char* lang);

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

    private:
        md_code_block_advanced(const md_entity& parent, const char* code, const char* lang);

        md_code_block_advanced(const md_entity& parent, cmark_node* node);

        friend detail::md_ptr_access;
    };

    class md_list_item;

    class md_inline_documentation final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::inline_documentation_t;
        }

        static md_ptr<md_inline_documentation> make(const md_entity&   parent,
                                                    const std::string& heading);

        md_entity& add_entity(md_entity_ptr entity) override;

        // adds the contents of all paragraphs in container
        // returns false if any child was not a paragraph
        bool add_item(const char* name, const char* id, const md_container& container);

        md_list_item& get_item() STANDARDESE_NOEXCEPT;

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

        md_ptr<md_document> clone() const
        {
            return md_ptr<md_document>(static_cast<md_document*>(do_clone(nullptr).release()));
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
