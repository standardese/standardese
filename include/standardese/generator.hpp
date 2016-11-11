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
        doc_ptr<doc_file>   file;
        md_ptr<md_document> document;

        documentation() = default;

        documentation(doc_ptr<doc_file> f, md_ptr<md_document> doc)
        : file(std::move(f)), document(std::move(doc))
        {
        }

        explicit documentation(md_ptr<md_document> doc) : file(nullptr), document(std::move(doc))
        {
        }
    };

    documentation generate_doc_file(const parser& p, const index& i, const cpp_file& f,
                                    std::string name);

    md_ptr<md_document> generate_file_index(index& i, std::string name = "standardese_files");

    md_ptr<md_document> generate_entity_index(index& i, std::string name = "standardese_entities");

    md_ptr<md_document> generate_module_index(const parser& p, index& i,
                                              std::string name = "standardese_modules");
} // namespace standardese

#endif // STANDARDESE_GENERATOR_HPP_INCLUDED
