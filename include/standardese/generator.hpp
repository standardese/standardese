// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_GENERATOR_HPP_INCLUDED
#define STANDARDESE_GENERATOR_HPP_INCLUDED

#include <vector>

#include <standardese/doc_entity.hpp>
#include <standardese/md_custom.hpp>
#include <standardese/translation_unit.hpp>

namespace standardese
{
    class parser;
    class index;

    struct documentation
    {
        doc_ptr<doc_entity> file;
        md_ptr<md_document> document;

        documentation() = default;

        documentation(doc_ptr<doc_entity> f, md_ptr<md_document> doc)
        : file(std::move(f)), document(std::move(doc))
        {
        }
    };

    documentation generate_doc_file(const parser& p, const index& i, const cpp_file& f,
                                    std::string name);

    class doc_index final : public doc_entity
    {
    protected:
        void do_generate_documentation(const parser&, const index&, md_document& doc,
                                       unsigned) const override
        {
            for (auto& child : *doc_)
                doc.add_entity(child.clone(doc));
        }

        void do_generate_synopsis(const parser&, code_block_writer&, bool) const
        {
        }

    private:
        doc_index(md_document& doc, std::string name)
        : doc_entity(doc_entity::index_t, nullptr, nullptr), name_(std::move(name)), doc_(&doc)
        {
        }

        cpp_name do_get_name() const override
        {
            return name_;
        }

        cpp_name do_get_unique_name() const override
        {
            return name_;
        }

        cpp_name do_get_index_name(bool) const override
        {
            return name_;
        }

        cpp_entity::type do_get_cpp_entity_type() const STANDARDESE_NOEXCEPT override
        {
            return cpp_entity::invalid_t;
        }

        std::string  name_;
        md_document* doc_;

        friend detail::doc_ptr_access;
    };

    documentation generate_file_index(index& i, std::string name = "standardese_files");

    documentation generate_entity_index(index& i, std::string name = "standardese_entities");

    documentation generate_module_index(const parser& p, index& i,
                                        std::string name = "standardese_modules");
} // namespace standardese

#endif // STANDARDESE_GENERATOR_HPP_INCLUDED
