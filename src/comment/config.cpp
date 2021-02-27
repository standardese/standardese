// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
//               2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment/config.hpp>
#include <stdexcept>
#include <cassert>

#include "../util/enum_values.hpp"

using namespace standardese::comment;

namespace {

// Return command_character, e.g., '\', as something that can be used in a
// regular expression, e.g., '\\'.
std::string command_character_escaped(char command_character) {
  std::string escaped = " ";
  escaped[0] = command_character;
  if (!std::regex_match(escaped, std::regex("\\w"))) {
    // Anything that is not alpha-numeric can be escaped with a backslash; it
    // probably does not need to be escaped though.
    escaped = "\\" + escaped;
  }
  return escaped;
}

const std::string eol = "(?:[[:space:]]*(?:\n|$))";
const std::string boundary = "(?:[[:space:]]+|" + eol + ")";
const std::string word = "[[:space:]]*([^[:space:]]+)" + boundary;
const std::string until_eol = "[[:space:]]*([^\n]*?)" + eol;

}

std::string config::default_command_pattern(char command_character, command_type cmd)
{
    const std::string prefix = command_character_escaped(command_character);
    const std::string end = "(?:" + prefix + "end" + boundary + ")";

    const std::string name = command_name(cmd);

    switch (cmd)
    {
    case command_type::end:
        return prefix + name + eol;
    case command_type::exclude:
        return prefix + name + boundary + "(target|return)?" + eol;
    case command_type::unique_name:
    case command_type::output_name:
    case command_type::module:
        return prefix + name + boundary + word + eol;
    case command_type::output_section:
    case command_type::entity:
    case command_type::synopsis:
        return prefix + name + boundary + until_eol;
    case command_type::group:
        return prefix + name + boundary + word + until_eol;
    case command_type::file:
        return prefix + name + boundary;
    default:
        throw std::logic_error("not implemented: unknown command type");
    }
}

std::string config::default_command_pattern(char command_character, section_type cmd)
{
    const std::string prefix = command_character_escaped(command_character);

    return prefix + command_name(cmd) + boundary;
}

std::string config::default_command_pattern(char command_character, inline_type cmd)
{
    const std::string prefix = command_character_escaped(command_character);

    return prefix + command_name(cmd) + boundary + word;
}

config::config(const options& options) : free_file_comments_(options.free_file_comments)
{
    const auto pattern = [&](const auto command) {
        const std::string name = command_name(command);
        const auto fallback = default_command_pattern(options.command_character, command);

        std::vector<std::string> parameters { name + "=" + fallback };

        for (const auto& specification : options.command_patterns)
            if (specification.rfind(name, 0) != std::string::npos)
                parameters.emplace_back(specification);

        return command_pattern(parameters);
    };

    for (const auto command : enum_values<command_type>())
        special_command_patterns_.emplace_back(pattern(command));
    for (const auto command : enum_values<section_type>())
        section_command_patterns_.emplace_back(pattern(command));
    for (const auto command : enum_values<inline_type>())
        inline_command_patterns_.emplace_back(pattern(command));
}

std::regex config::command_pattern(const std::vector<std::string>& options)
{
    if (options.size() == 0)
        throw std::invalid_argument("expected at least one pattern to merge");

    std::vector<std::string> patterns;

    for (const auto& option : options) {
        const auto split = option.find('=');
        if (split == std::string::npos || split == 0)
            throw std::invalid_argument("argument must be of the form `name=pattern` but `" + option + "` is not.");

        const bool merge = option[split - 1] == '|';

        const auto pattern = option.substr(split + 1);

        if (!merge)
            patterns.clear();

        patterns.emplace_back(std::move(pattern));
    }

    assert(patterns.size() != 0);

    if (patterns.size() == 1)
        return std::regex(*begin(patterns));

    std::string combined;
    for (const auto& pattern : patterns) {
        if (combined.size() != 0)
            combined += "|";
        combined += "(?:" + pattern + ")";
    }

    return std::regex(combined);
}

const char* config::command_name(command_type cmd) {
    switch(cmd) {
        case command_type::output_name:
            return "output_name";
        case command_type::output_section:
            return "output_section";
        case command_type::end:
            return "end";
        case command_type::entity:
            return "entity";
        case command_type::exclude:
            return "exclude";
        case command_type::file:
            return "file";
        case command_type::group:
            return "group";
        case command_type::module:
            return "module";
        case command_type::synopsis:
            return "synopsis";
        case command_type::unique_name:
            return "unique_name";
        default:
            throw std::logic_error("not implemented: unknown command type");
    }
}

const char* config::command_name(section_type cmd) {
    switch(cmd) {
        case section_type::brief:
            return "brief";
        case section_type::details:
            return "details";
        case section_type::requires:
            return "requires";
        case section_type::effects:
            return "effects";
        case section_type::synchronization:
            return "synchronization";
        case section_type::postconditions:
            return "postconditions";
        case section_type::returns:
            return "returns";
        case section_type::throws:
            return "throws";
        case section_type::complexity:
            return "complexity";
        case section_type::remarks:
            return "remarks";
        case section_type::error_conditions:
            return "error_conditions";
        case section_type::notes:
            return "notes";
        case section_type::preconditions:
            return "preconditions";
        case section_type::constraints:
            return "constraints";
        case section_type::diagnostics:
            return "diagnostics";
        case section_type::see:
            return "see";
        default:
            throw std::logic_error("not implemented: unknown section type");
    }
}

const char* config::command_name(inline_type cmd) {
    switch(cmd) {
        case inline_type::base:
            return "base";
        case inline_type::param:
            return "param";
        case inline_type::tparam:
            return "tparam";
        default:
            throw std::logic_error("not implemented: unsupported inline type");
    }
}

const std::regex& config::get_command_pattern(command_type cmd) const
{
    return special_command_patterns_[unsigned(cmd)];
}

const std::regex& config::get_command_pattern(section_type section) const
{
    return section_command_patterns_[unsigned(section)];
}

const std::regex& config::get_command_pattern(inline_type type) const
{
    return inline_command_patterns_[unsigned(type)];
}

const char* config::inline_section_name(section_type section) const
{
    switch (section)
    {
    case section_type::brief:
    case section_type::details:
        return "";

    case section_type::requires:
        return "Requires";
    case section_type::effects:
        return "Effects";
    case section_type::synchronization:
        return "Synchronization";
    case section_type::postconditions:
        return "Postconditions";
    case section_type::returns:
        return "Return values";
    case section_type::throws:
        return "Throws";
    case section_type::complexity:
        return "Complexity";
    case section_type::remarks:
        return "Remarks";
    case section_type::error_conditions:
        return "Error conditions";
    case section_type::notes:
        return "Notes";
    case section_type::preconditions:
        return "Preconditions";
    case section_type::constraints:
        return "Constraints";
    case section_type::diagnostics:
        return "Diagnostics";
    case section_type::see:
        return "See also";
    case section_type::count:
    default:
        throw std::logic_error("not implemented: unsupported section type");
    }
}
