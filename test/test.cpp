// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include <catch.hpp>

#include "test_parser.hpp"

const std::shared_ptr<spdlog::logger> test_logger = spdlog::stderr_logger_mt("test_logger");
