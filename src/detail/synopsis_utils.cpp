// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/synopsis_utils.hpp>

#include <standardese/cpp_class.hpp>
#include <standardese/cpp_function.hpp>

using namespace standardese;

void detail::write_type_value_default(const parser& par, code_block_writer& out,
                                      const cpp_type_ref& type, const cpp_name& name,
                                      const std::string& def, bool variadic)
{
    std::string type_name = get_ref_name(par, type).c_str();

    if (!name.empty())
    {
        auto pos = type_name.find("(*");
        if (pos != std::string::npos)
        {
            for (auto i = 0u; i <= pos + 1; ++i)
                out << type_name[i];

            if (variadic)
                out << " ... ";
            out << name;

            for (auto i = pos + 2; i != type_name.size(); ++i)
                out << type_name[i];
        }
        else
        {
            pos = type_name.find('[');
            if (pos != std::string::npos)
            {
                for (auto i = 0u; i != pos; ++i)
                    out << type_name[i];
                if (variadic)
                    out << " ...";
                out << ' ' << name;
                for (auto i = pos; i != type_name.size(); ++i)
                    out << type_name[i];
            }
            else
            {
                out << type_name;
                if (variadic)
                    out << " ...";
                out << ' ' << name;
            }
        }
    }
    else
    {
        out << type_name;
        if (variadic)
            out << " ...";
    }

    if (!def.empty())
        out << " = " << def;
}

void detail::write_class_name(code_block_writer& out, const cpp_name& name, int class_type)
{
    switch (class_type)
    {
    case cpp_struct_t:
        out << "struct ";
        break;
    case cpp_class_t:
        out << "class ";
        break;
    case cpp_union_t:
        out << "union ";
    }

    out << name;
}

void detail::write_bases(const parser& par, code_block_writer& out, const cpp_class& c,
                         bool extract_private)
{
    auto comma = false;
    for (auto& base : c.get_bases())
    {
        if (!extract_private && base.get_access() == cpp_private)
            continue;
        auto comment = par.get_comment_registry().lookup_comment(par.get_entity_registry(), base);
        if (comment && comment->is_excluded())
            continue;

        if (comma)
            out << ", ";
        else
        {
            comma = true;
            out << newl << ": ";
        }

        switch (base.get_access())
        {
        case cpp_public:
            if (c.get_class_type() == cpp_class_t)
                out << "public ";
            break;
        case cpp_private:
            if (c.get_class_type() != cpp_class_t)
                out << "private ";
            break;
        case cpp_protected:
            out << "protected ";
            break;
        }

        out << get_ref_name(par, base.get_type());
    }

    if (comma)
        out << newl;
}

void detail::write_parameters(const parser& par, code_block_writer& out, const cpp_function_base& f,
                              const cpp_name& override_name)
{
    if (override_name.empty())
        out << f.get_name();
    else
        out << override_name;
    out << '(';

    auto need = false;
    for (auto& p : f.get_parameters())
    {
        if (is_blacklisted(par, p))
            continue;
        else if (need)
        {
            out << ", ";
            need = false;
        }

        detail::write_type_value_default(par, out, p.get_type(), p.get_name(),
                                         p.get_default_value());
        need = true;
    }

    if (f.is_variadic())
    {
        if (!f.get_parameters().empty())
            out << ", ";
        out << "...";
    }

    out << ')';
}

void detail::write_noexcept(code_block_writer& out, const cpp_function_base& f)
{
    if (f.explicit_noexcept())
    {
        if (f.get_noexcept() == "true")
            out << " noexcept";
        else
            out << " noexcept(" << f.get_noexcept() << ')';
    }
}

void detail::write_definition(code_block_writer& out, const cpp_function_base& f)
{
    switch (f.get_definition())
    {
    case cpp_function_declaration:
    case cpp_function_definition_normal:
        out << ';';
        break;
    case cpp_function_definition_defaulted:
        out << " = default;";
        break;
    case cpp_function_definition_deleted:
        out << " = delete;";
        break;
    case cpp_function_definition_pure:
        out << " = 0";
        break;
    }
}

void detail::write_cv_ref(code_block_writer& out, int cv, int ref)
{
    if (cv & cpp_cv_const)
        out << " const";
    if (cv & cpp_cv_volatile)
        out << " volatile";

    if (ref == cpp_ref_rvalue)
        out << " &&";
    else if (ref == cpp_ref_lvalue)
        out << " &";
}

void detail::write_prefix(code_block_writer& out, int virtual_flag, bool constexpr_f,
                          bool explicit_f)
{
    if (virtual_flag == cpp_virtual_static)
        out << "static ";
    else if (is_virtual(cpp_virtual(virtual_flag)))
        out << "virtual ";

    if (explicit_f)
        out << "explicit ";

    if (constexpr_f)
        out << "constexpr ";
}

void detail::write_override_final(code_block_writer& out, int virtual_flag)
{
    if (virtual_flag == cpp_virtual_final)
        out << " final";
    else if (is_overriden(cpp_virtual(virtual_flag)))
        out << " override";
}
