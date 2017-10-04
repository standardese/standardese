// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_TEST_LOGGER_HPP_INCLUDED
#define STANDARDESE_TEST_LOGGER_HPP_INCLUDED

#include <catch.hpp>

#include <cppast/diagnostic_logger.hpp>

inline type_safe::object_ref<const cppast::diagnostic_logger> test_logger()
{
    class test_logger_t : public cppast::diagnostic_logger
    {
        bool do_log(const char* source, const cppast::diagnostic& d) const override
        {
            cppast::default_logger()->log(source, d);
            FAIL("diagnostic logged");
            return true;
        }
    };

    static test_logger_t logger;
    return type_safe::ref(logger);
}

#endif // STANDARDESE_TEST_LOGGER_HPP_INCLUDED
