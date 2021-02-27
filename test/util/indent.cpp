// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <boost/algorithm/string/replace.hpp>

#include "indent.hpp"

namespace standardese::test::util
{

std::string unindent(std::string comment) {
    if (comment.size() == 0) return comment;
    if (comment[0] != '\n') comment = '\n' + comment; 
    const auto indent = comment.find_first_not_of(' ', 1);
    if (indent != 1 && indent != std::string::npos)
        boost::algorithm::replace_all(comment, '\n' + std::string(indent - 1, ' '), "\n");
    return comment.substr(1);
}

}
