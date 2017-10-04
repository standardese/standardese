// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_LOGGER_HPP_INCLUDED
#define STANDARDESE_LOGGER_HPP_INCLUDED

#include <sstream>

#include <cppast/diagnostic_logger.hpp>

namespace standardese
{
    namespace detail
    {
        template <typename... Args>
        std::string format(Args&&... args)
        {
            std::ostringstream stream;
            int                dummy[] = {(stream << std::forward<Args>(args), 0)...};
            (void)dummy;
            return stream.str();
        }
    } // namespace detail

    /// Creates a diagnostic.
    /// \returns A diagnostic whose message are the arguments converted to a string for the specified location
    /// with severity warning.
    template <typename... Args>
    cppast::diagnostic make_diagnostic(cppast::source_location location, Args&&... args)
    {
        return {detail::format(std::forward<Args>(args)...), std::move(location),
                cppast::severity::warning};
    }
} // namespace standardese

#endif // STANDARDESE_LOGGER_HPP_INCLUDED
