// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/synopsis.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/cpp_preprocessor.hpp>
#include <standardese/cpp_type.hpp>
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
        out << newl << '}' << newl;
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
