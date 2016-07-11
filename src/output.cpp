// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/output.hpp>

#include <standardese/generator.hpp>

using namespace standardese;

void output::render(const md_document& document)
{
    file_output output(prefix_ + document.get_output_name() + '.' + format_->extension());
    format_->render(output, document);
}
