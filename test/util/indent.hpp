// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_TEST_UTIL_INDENT_HPP_INCLUDED
#define STANDARDESE_TEST_UTIL_INDENT_HPP_INCLUDED

#include <string>

#include <boost/algorithm/string/replace.hpp>

namespace standardese::test::util {

/// Return `comment` with common whitespace removed from the start of each line.
std::string unindent(std::string comment);

}

#endif
