// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/synopsis.hpp>

#include <standardese/cpp_class.hpp>
#include <standardese/cpp_enum.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/cpp_preprocessor.hpp>
#include <standardese/cpp_type.hpp>
#include <standardese/cpp_variable.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

namespace
{
    constexpr auto tab_width = 4;

    void dispatch(output_base::code_block_writer &out, const cpp_entity &e, bool top_level);

    template <class Container, typename T>
    void print_range(output_base::code_block_writer &out, const Container &cont, T sep)
    {
        auto need = false;
        for (auto& e : cont)
        {
            if (need)
                out << sep;
            else
                need = true;

            dispatch(out, e, false);
        }
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_file &f, bool)
    {
        print_range(out, f, blankl);
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_inclusion_directive &i, bool)
    {
        out << "#include ";

        if (i.get_kind() == cpp_inclusion_directive::local)
            out << '"';
        else
            out << '<';

        out << i.get_name();

        if (i.get_kind() == cpp_inclusion_directive::local)
            out << '"';
        else
            out << '>';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_macro_definition &m, bool)
    {
        out << "#define " << m.get_name() << m.get_argument_string() << ' ' << m.get_replacement();
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_namespace &ns, bool)
    {
        if (ns.is_inline())
            out << "inline ";
        out << "namespace " << ns.get_name() << newl;
        out << '{' << newl;
        out.indent(tab_width);

        print_range(out, ns, blankl);

        out.unindent(tab_width);
        out << newl << '}';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_namespace_alias &ns, bool)
    {
        out << "namespace " << ns.get_name() << " = " << ns.get_target() << ';';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_using_directive &u, bool)
    {
        out << "using namespace " << u.get_name() << ';';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_using_declaration &u, bool)
    {
        out << "using " << u.get_name() << ';';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_type_alias &a, bool)
    {
        out << "using " << a.get_name() << " = " << a.get_target().get_name() << ';';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_enum_value &e, bool)
    {
        out << e.get_name();
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_signed_enum_value &e, bool)
    {
        out << e.get_name();
        if (e.is_explicitly_given())
            out << " = " << e.get_value();
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_unsigned_enum_value &e, bool)
    {
        out << e.get_name();
        if (e.is_explicitly_given())
            out << " = " << e.get_value();
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_enum &e, bool top_level)
    {
        out << "enum ";
        if (e.is_scoped())
            out << "class ";
        out << e.get_name();
        if (top_level)
        {
            out << newl;
            if (!e.get_underlying_type().get_name().empty())
                out << ": " << e.get_underlying_type().get_name() << newl;
            out << '{' << newl;
            out.indent(tab_width);

            print_range(out, e, ",\n");

            out.unindent(tab_width);
            out << newl << '}';
        }
        else
            out << ';';
    }

    void write_class_name(output_base::code_block_writer &out, const cpp_class &c)
    {
        switch (c.get_class_type())
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

        out << c.get_name();
    }

    void write_bases(output_base::code_block_writer &out, const cpp_class &c)
    {
        auto comma = false;
        for (auto& base : c)
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

            switch (static_cast<const cpp_base_class&>(base).get_access())
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

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_access_specifier &a, bool)
    {
        out.unindent(tab_width);
        out << newl << a.get_name() << ':' << newl;
        out.indent(tab_width);
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_class &c, bool top_level)
    {
        write_class_name(out, c);

        if (top_level)
        {
            if (c.is_final())
                out << " final";
            out << newl;

            write_bases(out, c);

            out << '{' << newl;
            out.indent(tab_width);

            auto need = false;
            for (auto& e : c)
            {
                if (need)
                    out << blankl;
                else
                    need = true;
                dispatch(out, e, false);
            }

            out.unindent(tab_width);
            out << newl << '}';
        }
        else
            out << ';';
    }

    void print_type_value(output_base::code_block_writer &out, const cpp_type_ref &ref, const cpp_name &name)
    {
        out << ref.get_name();
        if (!name.empty())
            out << ' ' << name;
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_variable &v, bool)
    {
        switch (v.get_linkage())
        {
            case cpp_external_linkage:
                out << "extern ";
                break;
            case cpp_internal_linkage:
                out << "static ";
                break;
            default:
                break;
        }

        if (v.is_thread_local())
            out << "thread_local ";

        print_type_value(out, v.get_type(), v.get_name());
        if (!v.get_initializer().empty())
            out << " = " << v.get_initializer();
        out << ';';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_bitfield &v, bool)
    {
        print_type_value(out, v.get_type(), v.get_name());
        out << " : " << (unsigned long long)v.no_bits();
        if (!v.get_initializer().empty())
            out << " = " << v.get_initializer();
        out << ';';
    }

    void dispatch(output_base::code_block_writer &out, const cpp_entity &e, bool top_level)
    {
        switch (e.get_entity_type())
        {
            #define STANDARDESE_DETAIL_HANDLE(name) \
        case cpp_entity::name##_t: \
            do_write_synopsis(out, static_cast<const cpp_##name&>(e), top_level); \
            break;

            STANDARDESE_DETAIL_HANDLE(file)

            STANDARDESE_DETAIL_HANDLE(inclusion_directive)
            STANDARDESE_DETAIL_HANDLE(macro_definition)

            STANDARDESE_DETAIL_HANDLE(namespace)
            STANDARDESE_DETAIL_HANDLE(namespace_alias)
            STANDARDESE_DETAIL_HANDLE(using_directive)
            STANDARDESE_DETAIL_HANDLE(using_declaration)

            STANDARDESE_DETAIL_HANDLE(type_alias)

            STANDARDESE_DETAIL_HANDLE(enum_value)
            STANDARDESE_DETAIL_HANDLE(signed_enum_value)
            STANDARDESE_DETAIL_HANDLE(unsigned_enum_value)
            STANDARDESE_DETAIL_HANDLE(enum)

            STANDARDESE_DETAIL_HANDLE(class)
            STANDARDESE_DETAIL_HANDLE(access_specifier)

            STANDARDESE_DETAIL_HANDLE(variable)
            STANDARDESE_DETAIL_HANDLE(member_variable)
            STANDARDESE_DETAIL_HANDLE(bitfield)

            #undef STANDARDESE_DETAIL_HANDLE

            default:
                break;
        }
    }
}

void standardese::write_synopsis(output_base &out, const cpp_entity &e)
{
    output_base::code_block_writer w(out);
    dispatch(w, e, true);
}
