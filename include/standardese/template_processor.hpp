// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_TEMPLATE_PROCESSOR_HPP_INCLUDED
#define STANDARDESE_TEMPLATE_PROCESSOR_HPP_INCLUDED

#include <string>

#include <standardese/index.hpp>
#include <standardese/parser.hpp>

namespace standardese
{
    enum class template_command
    {
        generate_doc,
        generate_synopsis,
        generate_doc_text,

        for_each,
        if_clause,
        else_if_clause,
        else_clause,
        end,

        count,
        invalid = count
    };

    enum class template_if_operation
    {
        name,
        first_child,
        has_children,
        inline_entity,
        member_group,

        count,
        invalid = count
    };

    class template_config
    {
    public:
        template_config();

        void set_delimiters(std::string begin, std::string end)
        {
            delimiter_begin_ = std::move(begin);
            delimiter_end_   = std::move(end);
        }

        const std::string& delimiter_begin() const STANDARDESE_NOEXCEPT
        {
            return delimiter_begin_;
        }

        const std::string& delimiter_end() const STANDARDESE_NOEXCEPT
        {
            return delimiter_end_;
        }

        void set_command(template_command cmd, std::string str);

        template_command get_command(const std::string& str) const;

        template_command try_get_command(const std::string& str) const STANDARDESE_NOEXCEPT;

        void set_operation(template_if_operation op, std::string str);

        template_if_operation get_operation(const std::string& str) const;

        template_if_operation try_get_operation(const std::string& str) const STANDARDESE_NOEXCEPT;

    private:
        std::string commands_[static_cast<std::size_t>(template_command::count)];
        std::string if_operations_[static_cast<std::size_t>(template_if_operation::count)];
        std::string delimiter_begin_, delimiter_end_;
    };

    std::string process_template(const parser& p, const index& i, const template_config& config,
                                 const std::string& input);
} // namespace standardese

#endif // STANDARDESE_TEMPLATE_PROCESSOR_HPP_INCLUDED
