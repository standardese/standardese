// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/config.hpp>


#include <spdlog/details/format.h>

#include <standardese/section.hpp>

using namespace standardese;

namespace
{
    const char* standards[int(cpp_standard::count)];

    void init_standards()
    {
        standards[int(cpp_standard::cpp_98)] = "-std=c++98";
        standards[int(cpp_standard::cpp_03)] = "-std=c++03";
        standards[int(cpp_standard::cpp_11)] = "-std=c++11";
        standards[int(cpp_standard::cpp_14)] = "-std=c++14";
    }

    auto standards_initializer = (init_standards(), 0);
}

const char* detail::to_option(cpp_standard standard) STANDARDESE_NOEXCEPT
{
    return standards[int(standard)];
}

std::string compile_config::include_directory(std::string s)
{
    return "-I" + std::move(s);
}

std::string compile_config::macro_definition(std::string s)
{
    return "-D" + std::move(s);
}

std::string compile_config::macro_undefinition(std::string s)
{
    return "-U" + std::move(s);
}

comment_config::comment_config()
: cmd_char_('\\')
{
    #define STANDARDESE_DETAIL_SET(type) \
        set_section_command(section_type::type, #type);

    STANDARDESE_DETAIL_SET(brief)
    STANDARDESE_DETAIL_SET(details)

    STANDARDESE_DETAIL_SET(requires)
    STANDARDESE_DETAIL_SET(effects)
    STANDARDESE_DETAIL_SET(synchronization)
    STANDARDESE_DETAIL_SET(postconditions)
    STANDARDESE_DETAIL_SET(returns)
    STANDARDESE_DETAIL_SET(throws)
    STANDARDESE_DETAIL_SET(complexity)
    STANDARDESE_DETAIL_SET(remarks)
    STANDARDESE_DETAIL_SET(error_conditions)
    STANDARDESE_DETAIL_SET(notes)

    #undef STANDARDESE_DETAIL_SET
}

section_type comment_config::try_get_section(const std::string &command) const STANDARDESE_NOEXCEPT
{
    auto iter = section_commands_.find(command);
    return iter == section_commands_.end() ? section_type::invalid : section_type(iter->second);
}

section_type comment_config::get_section(const std::string &command) const
{
    auto iter = section_commands_.find(command);
    if (iter == section_commands_.end())
        throw std::invalid_argument(fmt::format("Invalid section command name '{}'", command));
    return section_type(iter->second);
}

void comment_config::set_section_command(section_type t, std::string command)
{
    // erase old command name
    for (auto iter = section_commands_.begin(); iter != section_commands_.end(); ++iter)
        if (iter->second == unsigned(t))
        {
            section_commands_.erase(iter);
            break;
        }

    // insert new name
    auto res = section_commands_.emplace(std::move(command), unsigned(t));
    if (!res.second)
        throw std::invalid_argument(fmt::format("Section command name '{}' already in use", command));
}

output_config::output_config()
: section_names_(std::size_t(section_type::count)),
  hidden_name_("implementation-defined"), tab_width_(4u)
{
    #define STANDARDESE_DETAIL_SET(type, name) \
        set_section_name(section_type::type, name);

    STANDARDESE_DETAIL_SET(brief, "")
    STANDARDESE_DETAIL_SET(details, "")

    STANDARDESE_DETAIL_SET(requires, "Requires")
    STANDARDESE_DETAIL_SET(effects, "Effects")
    STANDARDESE_DETAIL_SET(synchronization, "Synchronization")
    STANDARDESE_DETAIL_SET(postconditions, "Postconditions")
    STANDARDESE_DETAIL_SET(returns, "Returns")
    STANDARDESE_DETAIL_SET(throws, "Throws")
    STANDARDESE_DETAIL_SET(complexity, "Complexity")
    STANDARDESE_DETAIL_SET(remarks, "Remarks")
    STANDARDESE_DETAIL_SET(error_conditions, "Error conditions")
    STANDARDESE_DETAIL_SET(notes, "Notes")

    #undef STANDARDESE_DETAIL_SET
}
