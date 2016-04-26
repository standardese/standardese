// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_TEST_PARSER_HPP_INCLUDED
#define STANDARDESE_TEST_PARSER_HPP_INCLUDED

#include <fstream>

#include <standardese/parser.hpp>
#include <standardese/translation_unit.hpp>

inline standardese::translation_unit parse(standardese::parser &p, const char *name, const char *code,
                                           const char *standard = standardese::cpp_standard::cpp_14)
{
    std::ofstream file(name);
    file << code;
    file.close();
    
    return p.parse(name, standard);
}

#endif // STANDARDESE_TEST_PARSER_HPP_INCLUDED
