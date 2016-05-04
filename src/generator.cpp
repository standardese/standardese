// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/generator.hpp>

#include <standardese/comment.hpp>
#include <standardese/synopsis.hpp>

using namespace standardese;

void standardese::generate_doc_entity(output_base &output, unsigned level, const cpp_entity &e)
{
    output_base::heading_writer(output, level) << e.get_name();

    write_synopsis(output, e);

    auto comment = comment::parser(e).finish();
    for (auto& sec : comment.get_sections())
    {
        output.write_section_heading(sec.name);
        output_base::paragraph_writer(output) << sec.body;
    }
}

void standardese::generate_doc_file(output_base &output, const cpp_file &f)
{
    output_base::heading_writer(output, 1) << "Header " << f.get_name() << " synopsis";
    write_synopsis(output, f);

    for (auto& e : f)
        generate_doc_entity(output, 2, e);
}
