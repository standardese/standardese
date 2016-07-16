// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_GENERATOR_HPP_INCLUDED
#define STANDARDESE_GENERATOR_HPP_INCLUDED

#include <standardese/synopsis.hpp>
#include <standardese/translation_unit.hpp>

namespace standardese
{
    class parser;
    class index;

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

    const char* get_entity_type_spelling(cpp_entity::type t);

    void generate_doc_entity(const parser& p, const index& i, md_document& document, unsigned level,
                             const doc_entity& e);

    md_ptr<md_document> generate_doc_file(const parser& p, const index& i, cpp_file& f,
                                          std::string name);

    md_ptr<md_document> generate_file_index(index& i, std::string name = "files");

    md_ptr<md_document> generate_entity_index(index& i, std::string name = "entities");
} // namespace standardese

#endif // STANDARDESE_GENERATOR_HPP_INCLUDED
