// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/synopsis_utils.hpp>

#include <standardese/cpp_class.hpp>
#include <standardese/cpp_function.hpp>
#include <standardese/cpp_template.hpp>

using namespace standardese;

namespace
{
    void remove_trailing_ws(std::string& str)
    {
        while (!str.empty() && std::isspace(str.back()))
            str.pop_back();
    }

    void remove_leading_ws(std::string& str)
    {
        while (!str.empty() && std::isspace(str.front()))
            str.erase(0, 1);
    }

    bool remove_suffix(std::string& str, const char* suffix)
    {
        auto length = std::strlen(suffix);
        if (str.length() >= length && str.compare(str.length() - length, length, suffix) == 0)
        {
            str.erase(str.length() - length);
            remove_trailing_ws(str);
            return true;
        }
        else
            return false;
    }

    bool remove_suffix(std::string& str, char c)
    {
        if (!str.empty() && str.back() == c)
        {
            str.pop_back();
            remove_trailing_ws(str);
            return true;
        }
        else
            return false;
    }

    bool remove_prefix(std::string& str, const char* prefix)
    {
        auto length = std::strlen(prefix);
        if (str.length() >= length && str.compare(0, length, prefix) == 0)
        {
            str.erase(0, length);
            remove_leading_ws(str);
            return true;
        }
        else
            return false;
    }

    void prepend(std::string& result, const char* str)
    {
        auto copy = std::move(result);
        result    = str;
        detail::append_token(result, std::move(copy));
    }

    bool remove_reference(std::string& main, std::string& suffix)
    {
        if (remove_suffix(main, '&'))
        {
            if (remove_suffix(main, '&'))
                prepend(suffix, "&&");
            else
                prepend(suffix, "&");
            return true;
        }
        else
            return false;
    }

    bool remove_pointer(std::string& main, std::string& suffix)
    {
        auto cv = cpp_cv_none;

        if (remove_suffix(main, "const"))
        {
            if (remove_suffix(main, "volatile"))
                cv = cpp_cv(cpp_cv_const | cpp_cv_volatile);
            else
                cv = cpp_cv_const;
        }
        else if (remove_suffix(main, "volatile"))
        {
            if (remove_suffix(main, "const"))
                cv = cpp_cv(cpp_cv_const | cpp_cv_volatile);
            else
                cv = cpp_cv_volatile;
        }

        if (remove_suffix(main, '*'))
        {
            std::string this_suffix = "*";
            if (is_const(cv))
                this_suffix += " const";
            if (is_volatile(cv))
                this_suffix += " volatile";

            prepend(suffix, this_suffix.c_str());
            return true;
        }
        else
        {
            assert(cv == cpp_cv_none);
            return false;
        }
    }

    std::string get_prefix(std::string& main)
    {
        std::string prefix;

        auto result = true;
        while (result)
        {
            if (remove_prefix(main, "constexpr "))
                prefix += "constexpr ";
            else if (remove_prefix(main, "const "))
                prefix += "const ";
            else if (remove_prefix(main, "volatile "))
                prefix += "volatile";
            else
                result = false;
        }

        return prefix;
    }
}

void detail::write_entity_ref_name(const parser& p, code_block_writer& out,
                                   const detail::ref_name& name)
{
    if (name.unique_name.empty())
    {
        auto external = p.get_external_linker().lookup(name.name.c_str());
        out.write_link(false, name.name, external, true);
    }
    else
        out.write_link(false, name.name, name.unique_name);
}

void detail::write_type_ref_name(const parser& p, code_block_writer& out, const ref_name& name)
{
    std::string main, suffix;

    main = name.name.c_str();
    remove_trailing_ws(main);
    remove_reference(main, suffix);
    while (remove_pointer(main, suffix))
    {
    }

    auto prefix = get_prefix(main);

    if (name.unique_name.empty())
    {
        auto external = p.get_external_linker().lookup(main);
        (out << prefix).write_link(false, main.c_str(), external, true) << suffix;
    }
    else
        (out << prefix).write_link(false, main.c_str(), name.unique_name) << suffix;
}

void detail::write_type_value_default(const parser& par, code_block_writer& out, bool top_level,
                                      const cpp_type_ref& type, const cpp_name& name,
                                      const cpp_name& unique_name, const std::string& def,
                                      bool variadic)
{
    auto type_name = get_ref_name(par, type);
    write_type_ref_name(par, out, type_name);
    if (variadic)
        out << " ...";

    if (!name.empty())
        (out << ' ').write_link(top_level, name, unique_name);

    if (!def.empty())
        out << " = " << def;
}

void detail::write_template_parameters(const parser& par, code_block_writer& out,
                                       const doc_container_cpp_entity& cont)
{
    assert(is_template(cont.get_cpp_entity_type()));

    out << "template <";

    auto first = true;
    for (auto& child : cont)
    {
        if (!is_template_parameter(child.get_cpp_entity_type()))
        {
            if (first)
                // no template parameter yet
                continue;
            else
                // all template parameters handled
                break;
        }
        else if (!is_blacklisted(par, child))
        {
            if (first)
                first = false;
            else
                out << ", ";

            detail::generation_access::do_generate_synopsis(child, par, out, false);
        }
    }

    out << ">" << newl;
}

void detail::write_class_name(code_block_writer& out, bool top_level, const cpp_name& name,
                              const cpp_name& unique_name, int class_type)
{
    switch (static_cast<cpp_class_type>(class_type))
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

    out.write_link(top_level, name, unique_name);
}

void detail::write_bases(const parser& par, code_block_writer& out,
                         const doc_container_cpp_entity& cont, const cpp_class& c)
{
    auto comma = false;
    for (auto& child : cont)
    {
        if (child.get_cpp_entity_type() != cpp_entity::base_class_t || is_blacklisted(par, child))
            continue;
        auto& doc_e = static_cast<const doc_cpp_entity&>(child);
        auto& base  = static_cast<const cpp_base_class&>(doc_e.get_cpp_entity());

        if (comma)
            out << ',' << newl << "  ";
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

        auto base_name = get_ref_name(par, base.get_type());
        write_entity_ref_name(par, out, base_name);
    }

    if (comma)
        out << newl;
}

void detail::write_parameters(const parser& par, code_block_writer& out, bool top_level,
                              const doc_container_cpp_entity& cont, const cpp_function_base& f)

{
    if (cont.get_cpp_entity_type() == cpp_entity::function_template_specialization_t)
        out.write_link(top_level, cont.get_cpp_entity().get_name(), cont.get_unique_name());
    else
        out.write_link(top_level, f.get_name(), cont.get_unique_name());

    out << '(';

    auto need = false;
    for (auto& child : cont)
    {
        if (child.get_cpp_entity_type() != cpp_entity::function_parameter_t
            || is_blacklisted(par, child))
            continue;
        auto& doc_e = static_cast<const doc_cpp_entity&>(child);
        auto& p     = static_cast<const cpp_function_parameter&>(doc_e.get_cpp_entity());

        if (need)
            out << ", ";
        else
            need = true;

        detail::write_type_value_default(par, out, false, p.get_type(), p.get_name(), "",
                                         p.get_default_value());
    }

    if (f.is_variadic())
    {
        if (!f.get_parameters().empty())
            out << ", ";
        out << "...";
    }

    out << ')';
}

void detail::write_noexcept(const char* complex_name, code_block_writer& out,
                            const cpp_function_base& f)
{
    if (f.explicit_noexcept())
    {
        if (f.get_noexcept() == "true")
            out << " noexcept";
        else if (f.get_noexcept() == "false")
            out << " noexcept(false)";
        else if (!complex_name)
            out << " noexcept(" << f.get_noexcept() << ')';
        else
            out << " noexcept(" << complex_name << ')';
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
    else if (virtual_flag == cpp_virtual_friend)
        out << "friend ";
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
