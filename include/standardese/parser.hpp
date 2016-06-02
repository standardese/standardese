// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_PARSER_HPP_INCLUDED
#define STANDARDESE_PARSER_HPP_INCLUDED

#include <clang-c/Index.h>
#include <memory>
#include <string>
#include <vector>

#include <spdlog/logger.h>

#include <standardese/detail/wrapper.hpp>
#include <standardese/cpp_entity.hpp>

namespace standardese
{
    class translation_unit;

    /// C++ standard to be used
    enum class cpp_standard
    {
        cpp_98,
        cpp_03,
        cpp_11,
        cpp_14,
        count
    };

    struct compile_config
    {
        cpp_standard standard;
        std::vector<std::string> options;
        std::string commands_dir; // if non-empty looks for a compile_commands.json specification

        static std::string include_directory(std::string s);

        static std::string macro_definition(std::string s);

        static std::string macro_undefinition(std::string s);

        compile_config(std::string commands_dir)
        : standard(cpp_standard::count), commands_dir(std::move(commands_dir)) {}

        compile_config(cpp_standard s, std::vector<std::string> options = {})
        : standard(s), options(std::move(options)) {}
    };

    /// Parser class used for parsing the C++ classes.
    /// The parser object must live as long as all the translation units.
    class parser
    {
    public:
        parser();

        explicit parser(std::shared_ptr<spdlog::logger> logger);

        parser(parser&&) = delete;
        parser(const parser&) = delete;

        ~parser() STANDARDESE_NOEXCEPT;

        parser& operator=(parser&&) = delete;
        parser& operator=(const parser&) = delete;

        /// Parses a translation unit.
        translation_unit parse(const char *path, const compile_config &c) const;

        const std::shared_ptr<spdlog::logger>& get_logger() const STANDARDESE_NOEXCEPT
        {
            return logger_;
        }

    private:
        struct deleter
        {
            void operator()(CXIndex idx) const STANDARDESE_NOEXCEPT;
        };

        detail::wrapper<CXIndex, deleter> index_;
        std::shared_ptr<spdlog::logger> logger_;
    };
} // namespace standardese

#endif // STANDARDESE_PARSER_HPP_INCLUDED
