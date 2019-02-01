// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment/config.hpp>

using namespace standardese::comment;

const char* config::default_command_name(command_type cmd) noexcept
{
    switch (cmd)
    {
    case command_type::verbatim:
        return "verbatim";
    case command_type::end:
        return "end";

    case command_type::exclude:
        return "exclude";
    case command_type::unique_name:
        return "unique_name";
    case command_type::output_name:
        return "output_name";
    case command_type::synopsis:
        return "synopsis";

    case command_type::group:
        return "group";
    case command_type::module:
        return "module";
    case command_type::output_section:
        return "output_section";

    case command_type::entity:
        return "entity";
    case command_type::file:
        return "file";

    case command_type::invalid:
    case command_type::count:
        break;
    }

    assert(false);
    return "invalid command type";
}

const char* config::default_command_name(section_type cmd) noexcept
{
    switch (cmd)
    {
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

    case section_type::count:
        break;
    }
    assert(false);
    return "invalid section type";
}

const char* config::default_command_name(inline_type cmd) noexcept
{
    switch (cmd)
    {
    case inline_type::param:
        return "param";
    case inline_type::tparam:
        return "tparam";
    case inline_type::base:
        return "base";

    case inline_type::invalid:
    case inline_type::count:
        break;
    }
    assert(false);
    return "invalid inline type";
}

const char* config::default_inline_section_name(section_type section) noexcept
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
        return "Returns";
    case section_type::throws:
        return "Throws";
    case section_type::complexity:
        return "Complexity";
    case section_type::remarks:
        return "Remarks";
    case section_type::error_conditions:
        return "Error Conditions";
    case section_type::notes:
        return "Notes";

    case section_type::preconditions:
        return "Preconditions";

    case section_type::constraints:
        return "Constraints";
    case section_type::diagnostics:
        return "Diagnostics";

    case section_type::see:
        return "See";

    case section_type::count:
        break;
    }
    assert(false);
    return "forgot to name a section";
}

const char* config::default_list_section_name(section_type section) noexcept
{
    switch (section)
    {
    case section_type::brief:
    case section_type::details:
        return "";

    case section_type::requires:
        return "Requirements";
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
        break;
    }
    assert(false);
    return "forgot a section";
}

config::config(char command_character) : command_character_(command_character)
{
    for (auto i = 0u; i != unsigned(section_type::count); ++i)
        command_names_[i] = default_command_name(make_section(i));
    for (auto i = unsigned(section_type::count) + 1u; i != unsigned(command_type::count); ++i)
        command_names_[i] = default_command_name(make_command(i));
    for (auto i = unsigned(command_type::count) + 1u; i != unsigned(inline_type::count); ++i)
        command_names_[i] = default_command_name(make_inline(i));

    for (auto i = 0u; i != unsigned(section_type::count); ++i)
        inline_sections_[i] = default_inline_section_name(make_section(i));

    for (auto i = 0u; i != unsigned(section_type::count); ++i)
        list_sections_[i] = default_list_section_name(make_section(i));
}

void config::set_command_name(command_type cmd, std::string name)
{
    command_names_[unsigned(cmd)] = std::move(name);
}

void config::set_command_name(section_type cmd, std::string name)
{
    command_names_[unsigned(cmd)] = std::move(name);
}

void config::set_command_name(inline_type cmd, std::string name)
{
    command_names_[unsigned(cmd)] = std::move(name);
}

const char* config::command_name(command_type cmd) const noexcept
{
    return command_names_[unsigned(cmd)].c_str();
}

const char* config::command_name(section_type cmd) const noexcept
{
    return command_names_[unsigned(cmd)].c_str();
}

const char* config::command_name(inline_type cmd) const noexcept
{
    return command_names_[unsigned(cmd)].c_str();
}

unsigned config::try_lookup(const char* name) const noexcept
{
    auto i = 0u;
    for (auto& el : command_names_)
    {
        if (el == name)
            return i;
        ++i;
    }

    return unsigned(command_type::invalid);
}

const char* config::inline_section_name(section_type section) const noexcept
{
    return inline_sections_[unsigned(section)].c_str();
}

const char* config::list_section_name(section_type section) const noexcept
{
    return list_sections_[unsigned(section)].c_str();
}
