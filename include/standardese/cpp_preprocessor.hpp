// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_PREPROCESSOR_HPP_INCLUDED
#define STANDARDESE_CPP_PREPROCESSOR_HPP_INCLUDED

#include <string>
#include <unordered_set>

#include <standardese/cpp_entity.hpp>
#include <standardese/noexcept.hpp>

namespace standardese
{
    class compile_config;
    class cpp_file;

    class cpp_inclusion_directive : public cpp_entity
    {
    public:
        enum kind
        {
            system,
            local
        };

        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::inclusion_directive_t;
        }

        static cpp_ptr<cpp_inclusion_directive> make(const cpp_entity& parent,
                                                     std::string file_name, kind k, unsigned line)
        {
            return detail::make_cpp_ptr<cpp_inclusion_directive>(parent, std::move(file_name), k,
                                                                 line);
        }

        cpp_name get_name() const override
        {
            return "inclusion directive";
        }

        kind get_kind() const STANDARDESE_NOEXCEPT
        {
            return kind_;
        }

        const std::string& get_file_name() const STANDARDESE_NOEXCEPT
        {
            return file_name_;
        }

        unsigned get_line_number() const STANDARDESE_NOEXCEPT
        {
            return line_;
        }

    private:
        cpp_inclusion_directive(const cpp_entity& parent, std::string file_name, kind k,
                                unsigned line)
        : cpp_entity(get_entity_type(), clang_getNullCursor(), parent),
          file_name_(std::move(file_name)),
          kind_(k),
          line_(line)
        {
        }

        std::string file_name_;
        kind        kind_;
        unsigned    line_;

        friend detail::cpp_ptr_access;
    };

    class cpp_macro_definition final : public cpp_entity
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::macro_definition_t;
        }

        static cpp_ptr<standardese::cpp_macro_definition> parse(CXTranslationUnit tu, CXFile file,
                                                                cpp_cursor        cur,
                                                                const cpp_entity& parent,
                                                                unsigned          line_no);

        cpp_name get_name() const override
        {
            return name_;
        }

        bool is_function_macro() const STANDARDESE_NOEXCEPT
        {
            return !params_.empty();
        }

        /// Returns the parameter string including brackets.
        const std::string& get_parameter_string() const STANDARDESE_NOEXCEPT
        {
            return params_;
        }

        const std::string& get_replacement() const STANDARDESE_NOEXCEPT
        {
            return replacement_;
        }

        unsigned get_line_number() const STANDARDESE_NOEXCEPT
        {
            return line_;
        }

    private:
        cpp_macro_definition(const cpp_entity& parent, std::string name, std::string params,
                             std::string replacement, unsigned line)
        : cpp_entity(get_entity_type(), clang_getNullCursor(), parent),
          name_(std::move(name)),
          params_(std::move(params)),
          replacement_(std::move(replacement)),
          line_(line)
        {
        }

        std::string name_, params_, replacement_;
        unsigned    line_;

        friend detail::cpp_ptr_access;
    };

    class preprocessor
    {
    public:
        std::string preprocess(const parser& p, const compile_config& c, const char* full_path,
                               cpp_file& file) const;

        void whitelist_include_dir(std::string dir);

        bool is_whitelisted_directory(std::string& dir) const STANDARDESE_NOEXCEPT;

    private:
        std::unordered_set<std::string> include_dirs_;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_PREPROCESSOR_HPP_INCLUDED
