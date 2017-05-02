// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/generator.hpp>

#include <fstream>
#include <sstream>

#include <standardese/markup/document.hpp>

using namespace standardese::markup;

std::string standardese::markup::render(generator gen, const entity& e)
{
    std::ostringstream stream;
    gen(stream, e);
    return stream.str();
}
