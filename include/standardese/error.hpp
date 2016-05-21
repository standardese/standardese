// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_ERROR_HPP_INCLUDED
#define STANDARDESE_ERROR_HPP_INCLUDED

#include <stdexcept>

#include <clang-c/Index.h>

#include <standardese/noexcept.hpp>

namespace standardese
{
    struct source_location
    {
        std::string entity_name, file_name;
        unsigned line;

        source_location(std::string entity, std::string file, unsigned line)
        : entity_name(std::move(entity)), file_name(std::move(file)), line(line)
        {}

        source_location(CXSourceLocation location, std::string entity);
    };

   class parse_error
   : public std::runtime_error
   {
   public:
       parse_error(source_location location, std::string message)
       : std::runtime_error(std::move(message)), location_(std::move(location))
       {}

       const source_location& get_location() const STANDARDESE_NOEXCEPT
       {
           return location_;
       }

   private:
       source_location location_;
   };
} // namespace standardese

#endif // STANDARDESE_ERROR_HPP_INCLUDED
