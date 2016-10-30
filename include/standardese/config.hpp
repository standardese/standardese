// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CONFIG_HPP_INCLUDED
#define STANDARDESE_CONFIG_HPP_INCLUDED

#include <map>
#include <string>
#include <vector>

#include <standardese/noexcept.hpp>
#include <standardese/string.hpp>
#include <standardese/synopsis.hpp>

namespace standardese
{
    /// C++ standard to be used
    enum class cpp_standard
    {
        cpp_98,
        cpp_03,
        cpp_11,
        cpp_14,
        count
    };

    /// Set of "special" libclang flags.
    enum class compile_flag
    {
        ms_compatibility,
        ms_extensions,
        count,
    };

    class compile_config
    {
    public:
        compile_config(cpp_standard standard, string commands_dir = "");

        void add_macro_definition(string def);

        void remove_macro_definition(string def);

        void add_include(string path);

        void set_flag(compile_flag f);

        // major version number
        void set_msvc_compatibility_version(unsigned version);

        void set_clang_binary(std::string path)
        {
            clang_binary_ = std::move(path);
        }

        const std::string& get_clang_binary() const
        {
            return clang_binary_;
        }

        std::vector<const char*> get_flags() const;

        std::vector<string>::const_iterator begin() const
        {
            return flags_.begin();
        }

        std::vector<string>::const_iterator end() const
        {
            return flags_.end();
        }

    private:
        std::vector<string> flags_;
        std::string         clang_binary_;
    };

    enum class command_type : unsigned;
    enum class section_type : unsigned;

    class comment_config
    {
    public:
        comment_config();

        void set_command_character(char c) STANDARDESE_NOEXCEPT
        {
            cmd_char_ = c;
        }

        char get_command_character() const STANDARDESE_NOEXCEPT
        {
            return cmd_char_;
        }

        void set_command(unsigned c, std::string command);

        unsigned get_command(const std::string& command) const;

        unsigned try_get_command(const std::string& command) const STANDARDESE_NOEXCEPT;

    private:
        std::map<std::string, unsigned> commands_;
        char cmd_char_;
    };

    class output_config
    {
    public:
        output_config();

        void set_section_name(section_type t, std::string name);

        const std::string& get_section_name(section_type t) const STANDARDESE_NOEXCEPT
        {
            return section_names_[unsigned(t)];
        }

        entity_blacklist& get_blacklist() STANDARDESE_NOEXCEPT
        {
            return blacklist_;
        }

        const entity_blacklist& get_blacklist() const STANDARDESE_NOEXCEPT
        {
            return blacklist_;
        }

        void set_tab_width(unsigned w) STANDARDESE_NOEXCEPT
        {
            tab_width_ = w;
        }

        unsigned get_tab_width() const STANDARDESE_NOEXCEPT
        {
            return tab_width_;
        }

        void set_hidden_name(std::string name)
        {
            hidden_name_ = std::move(name);
        }

        const std::string& get_hidden_name() const STANDARDESE_NOEXCEPT
        {
            return hidden_name_;
        }

        bool inline_documentation() const STANDARDESE_NOEXCEPT
        {
            return inline_doc_;
        }

        void set_inline_documentation(bool v) STANDARDESE_NOEXCEPT
        {
            inline_doc_ = v;
        }

    private:
        entity_blacklist         blacklist_;
        std::vector<std::string> section_names_;
        std::string              hidden_name_;
        unsigned                 tab_width_;
        bool                     inline_doc_;
    };
} // namespace standardese

#endif // STANDARDESE_CONFIG_HPP_INCLUDED
