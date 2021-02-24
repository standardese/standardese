// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
//               2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_CONFIG_HPP_INCLUDED
#define STANDARDESE_COMMENT_CONFIG_HPP_INCLUDED

#include <array>
#include <string>
#include <regex>

#include <standardese/comment/commands.hpp>

namespace standardese
{
namespace comment
{
    /// The configuration of related to the comment syntax.
    class config
    {
    public:
        /// \returns The default command character.
        static char default_command_character()
        {
            return '\\';
        }

        /// \returns The default value for whether comments should be applied
        /// to the entire file if they cannot be matched elsewhere.
        static bool default_free_file_comments()
        {
            return false;
        }

        /// \effects Create a configuration such that all commands are formed
        /// by prefixing the command name with the command character.
        explicit config(
            char command_character = default_command_character(),
            bool free_file_comments = default_free_file_comments(),
            const std::vector<std::string>& command_patterns = {});

        /// \returns The pattern that introduces a cmd command.
        const std::regex& get_command_pattern(command_type cmd) const;

        /// \returns The pattern that introduces a cmd section.
        const std::regex& get_command_pattern(section_type cmd) const;

        /// \returns The pattern that introduces a cmd inline.
        const std::regex& get_command_pattern(inline_type cmd) const;

        /// \returns The name of a [standardese::markup::inline_section]().
        const char* inline_section_name(section_type section) const;

        /// \returns Whether comments in a file that cannot be associated to a
        /// particular entity such as a class should be treated as comments for
        /// the entire header file even if they do not start with the `\file`
        /// command.
        bool free_file_comments() const {
            return free_file_comments_;
        }

    private:

        /// \returns The default pattern for the given command or section.
        /// \group default_command_pattern
        static std::string default_command_pattern(char command_character, command_type cmd);

        /// \group default_command_pattern
        static std::string default_command_pattern(char command_character, section_type cmd);

        /// \group default_command_pattern
        static std::string default_command_pattern(char command_character, inline_type cmd);

        /// \returns The default name of this command, i.e., the `name` in `\name`.
        static const char* command_name(command_type cmd);

        /// \returns The default name of this command, i.e., the `name` in `\name`.
        static const char* command_name(section_type cmd);

        /// \returns The default name of this command, i.e., the `name` in `\name`.
        static const char* command_name(inline_type cmd);

        /// \returns The pattern obtained from the command line arguments `options`.
        static std::regex command_pattern(const std::vector<std::string>& options);

        std::vector<std::regex> special_command_patterns_;
        std::vector<std::regex> section_command_patterns_;
        std::vector<std::regex> inline_command_patterns_;

        bool free_file_comments_;
    };
} // namespace comment
} // namespace standardese

#endif // STANDARDESE_COMMENT_CONFIG_HPP_INCLUDED
