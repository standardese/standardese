// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/synopsis_utils.hpp>

#include <standardese/cpp_class.hpp>
#include <standardese/cpp_function.hpp>

using namespace standardese;

void detail::write_type_value_default(output_base::code_block_writer &out,
                              const cpp_type_ref &type, const cpp_name &name,
                              const std::string &def)
{
    if (!name.empty())
    {
        auto &type_name = type.get_name();
        auto pos = type_name.find("(*");
        if (pos != std::string::npos)
        {
            for (auto i = 0u; i <= pos + 1; ++i)
                out << type_name[i];
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
                out << ' ' << name;
                for (auto i = pos; i != type_name.size(); ++i)
                    out << type_name[i];
            }
            else
                out << type_name << ' ' << name;
        }
    }
    else
        out << type.get_name();

    if (!def.empty())
        out << " = " << def;
}

void detail::write_class_name(output_base::code_block_writer &out,
                      const cpp_name &name, int class_type)
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

void detail::write_bases(output_base::code_block_writer &out, const cpp_class &c)
{
    auto comma = false;
    for (auto &base : c)
    {
        if (base.get_entity_type() != cpp_entity::base_class_t)
            break;

        if (comma)
            out << ", ";
        else
        {
            comma = true;
            out << ": ";
        }

        switch (static_cast<const cpp_base_class &>(base).get_access())
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

        out << base.get_name();
    }

    if (comma)
        out << newl;
}

namespace
{
    void write_parameter(output_base::code_block_writer &out, const cpp_function_parameter &p)
    {
        detail::write_type_value_default(out, p.get_type(), p.get_name(), p.get_default_value());
    }
}

void detail::write_parameters(output_base::code_block_writer &out, const cpp_function_base &f,
                      const cpp_name &override_name)
{
    if (override_name.empty())
        out << f.get_name();
    else
        out << override_name;
    out << '(';

    write_range(out, f.get_parameters(), ", ", write_parameter);

    if (f.is_variadic())
    {
        if (!f.get_parameters().empty())
            out << ", ";
        out << "...";
    }

    out << ')';
}

void detail::write_noexcept(output_base::code_block_writer &out, const cpp_function_base &f)
{
    if (f.explicit_noexcept())
        out << " noexcept(" << f.get_noexcept() << ')';
}

void detail::write_definition(output_base::code_block_writer &out, const cpp_function_base &f, bool pure)
{
    if (pure)
    {
        out << " = 0";
        assert(f.get_definition() == cpp_function_definition_normal);
    }

    switch (f.get_definition())
    {
        case cpp_function_definition_normal:
            out << ';';
            break;
        case cpp_function_definition_defaulted:
            out << " = default;";
            break;
        case cpp_function_definition_deleted:
            out << " = delete";
            break;
    }
}

void detail::write_cv_ref(output_base::code_block_writer &out, int cv, int ref)
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

void detail::write_prefix(output_base::code_block_writer &out, int virtual_flag, bool constexpr_f, bool explicit_f)
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

void detail::write_override_final(output_base::code_block_writer &out, int virtual_flag)
{
    if (virtual_flag == cpp_virtual_final)
        out << " final";
    else if (is_overriden(cpp_virtual(virtual_flag)))
        out << " override";
}
