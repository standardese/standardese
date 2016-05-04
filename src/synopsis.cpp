// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/synopsis.hpp>

#include <standardese/cpp_class.hpp>
#include <standardese/cpp_enum.hpp>
#include <standardese/cpp_function.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/cpp_preprocessor.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/cpp_type.hpp>
#include <standardese/cpp_variable.hpp>
#include <standardese/translation_unit.hpp>

#include <standardese/detail/synopsis_utils.hpp>

using namespace standardese;

namespace
{
    constexpr auto tab_width = 4;

    void dispatch(output_base::code_block_writer &out, const cpp_entity &e, bool top_level,
                  const cpp_name &override_name = "");

    void write_entity(output_base::code_block_writer &out, const cpp_entity &e)
    {
        dispatch(out, e, false);
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_file &f)
    {
        detail::write_range(out, f, blankl, write_entity);
    }

    //=== preprocessor ===//
    void do_write_synopsis(output_base::code_block_writer &out, const cpp_inclusion_directive &i)
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

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_macro_definition &m)
    {
        out << "#define " << m.get_name() << m.get_argument_string() << ' ' << m.get_replacement();
    }

    //=== namespace related ===//
    void do_write_synopsis(output_base::code_block_writer &out, const cpp_namespace &ns)
    {
        if (ns.is_inline())
            out << "inline ";
        out << "namespace " << ns.get_name() << newl;
        out << '{' << newl;
        out.indent(tab_width);

        detail::write_range(out, ns, blankl, write_entity);

        out.unindent(tab_width);
        out << newl << '}';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_namespace_alias &ns)
    {
        out << "namespace " << ns.get_name() << " = " << ns.get_target() << ';';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_using_directive &u)
    {
        out << "using namespace " << u.get_name() << ';';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_using_declaration &u)
    {
        out << "using " << u.get_name() << ';';
    }

    //=== types ===//
    void do_write_synopsis(output_base::code_block_writer &out, const cpp_type_alias &a)
    {
        out << "using " << a.get_name() << " = " << a.get_target().get_name() << ';';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_enum_value &e)
    {
        out << e.get_name();
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_signed_enum_value &e)
    {
        out << e.get_name();
        if (e.is_explicitly_given())
            out << " = " << e.get_value();
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_unsigned_enum_value &e)
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

            detail::write_range(out, e, newl, write_entity);

            out.unindent(tab_width);
            out << newl << '}';
        }
        out << ';';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_access_specifier &a)
    {
        out.unindent(tab_width);
        out << newl << a.get_name() << ':' << newl;
        out.indent(tab_width);
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_class &c,
                           bool top_level, const cpp_name &override_name)
    {
        detail::write_class_name(out,
                                 override_name.empty() ? c.get_name() : override_name,
                                 c.get_class_type());

        if (top_level)
        {
            if (c.is_final())
                out << " final";
            out << newl;

            detail::write_bases(out, c);

            out << '{' << newl;
            out.indent(tab_width);

            detail::write_range(out, c, blankl, write_entity);

            out.unindent(tab_width);
            out << newl << '}';
        }

        out << ';';
    }

    //=== variables ===//
    void do_write_synopsis(output_base::code_block_writer &out, const cpp_variable &v)
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

        detail::write_type_value_default(out, v.get_type(), v.get_name(), v.get_initializer());
        out << ';';
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_bitfield &v)
    {
        detail::write_type_value_default(out, v.get_type(), v.get_name());
        out << " : " << (unsigned long long) v.no_bits();
        if (!v.get_initializer().empty())
            out << " = " << v.get_initializer();
        out << ';';
    }

    //=== functions ===//
    void do_write_synopsis(output_base::code_block_writer &out, const cpp_function &f,
                           bool, const cpp_name &override_name)
    {
        if (f.is_constexpr())
            out << "constexpr ";

        out << f.get_return_type().get_name() << ' ';
        detail::write_parameters(out, f, override_name);
        detail::write_noexcept(out, f);
        detail::write_definition(out, f);
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_member_function &f,
                           bool, const cpp_name &override_name)
    {
        detail::write_prefix(out, f.get_virtual(), f.is_constexpr());
        out << f.get_return_type().get_name() << ' ';
        detail::write_parameters(out, f, override_name);
        detail::write_cv_ref(out, f.get_cv(), f.get_ref_qualifier());
        detail::write_noexcept(out, f);
        detail::write_override_final(out, f.get_virtual());
        detail::write_definition(out, f, f.get_virtual() == cpp_virtual_pure);
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_conversion_op &f,
                           bool, const cpp_name &override_name)
    {
        detail::write_prefix(out, f.get_virtual(), f.is_constexpr(), f.is_explicit());
        detail::write_parameters(out, f, override_name);
        detail::write_cv_ref(out, f.get_cv(), f.get_ref_qualifier());
        detail::write_noexcept(out, f);
        detail::write_override_final(out, f.get_virtual());
        detail::write_definition(out, f, f.get_virtual() == cpp_virtual_pure);
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_constructor &f,
                           bool, const cpp_name &override_name)
    {
        detail::write_prefix(out, cpp_virtual_none, f.is_constexpr(), f.is_explicit());
        detail::write_parameters(out, f, override_name);
        detail::write_noexcept(out, f);
        detail::write_definition(out, f);
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_destructor &f,
                           bool, const cpp_name &override_name)
    {
        detail::write_prefix(out, f.get_virtual(), f.is_constexpr());
        detail::write_parameters(out, f, override_name);
        detail::write_noexcept(out, f);
        detail::write_definition(out, f, f.get_virtual() == cpp_virtual_pure);
    }

    //=== templates ===//
    void do_write_synopsis(output_base::code_block_writer &out, const cpp_template_type_parameter &p)
    {
        out << "typename";
        if (!p.get_name().empty())
            out << ' ' << p.get_name();
        if (p.has_default_type())
            out << " = " << p.get_default_type().get_name();
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_non_type_template_parameter &p)
    {
        detail::write_type_value_default(out, p.get_type(), p.get_name(), p.get_default_value());
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_template_template_parameter &p)
    {
        out << "template <";

        detail::write_range(out, p, ", ", write_entity);

        out << "> typename";
        if (!p.get_name().empty())
            out << ' ' << p.get_name();
        if (p.has_default_template())
            out << " = " << p.get_default_template().get_name();
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_function_template &f)
    {
        out << "template <";

        detail::write_range(out, f.get_template_parameters(), ", ", write_entity);

        out << '>' << newl;

        dispatch(out, f.get_function(), false);
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_function_template_specialization &f)
    {
        out << "template <>" << newl;
        dispatch(out, f.get_function(), false, f.get_name());
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_class_template &c)
    {
        out << "template <";

        detail::write_range(out, c.get_template_parameters(), ", ", write_entity);

        out << '>' << newl;

        dispatch(out, c.get_class(), false);
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_class_template_full_specialization &c)
    {
        out << "template <>" << newl;
        dispatch(out, c.get_class(), false, c.get_name());
    }

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_class_template_partial_specialization &c)
    {
        out << "template <";

        detail::write_range(out, c.get_template_parameters(), ", ", write_entity);

        out << '>' << newl;

        dispatch(out, c.get_class(), false, c.get_name());
    }

    //=== dispatching ===//
    template <typename T>
    void do_write_synopsis(output_base::code_block_writer &out, const T &e, bool)
    {
        do_write_synopsis(out, e);
    }

    template <typename T>
    void do_write_synopsis(output_base::code_block_writer &out, const T &e, bool top_level, const cpp_name &)
    {
        do_write_synopsis(out, e, top_level);
    }

    void dispatch(output_base::code_block_writer &out, const cpp_entity &e, bool top_level,
                  const cpp_name &override_name)
    {
        switch (e.get_entity_type())
        {
            #define STANDARDESE_DETAIL_HANDLE(name) \
        case cpp_entity::name##_t: \
            do_write_synopsis(out, static_cast<const cpp_##name&>(e), top_level, override_name); \
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

            STANDARDESE_DETAIL_HANDLE(function)
            STANDARDESE_DETAIL_HANDLE(member_function)
            STANDARDESE_DETAIL_HANDLE(conversion_op)
            STANDARDESE_DETAIL_HANDLE(constructor)
            STANDARDESE_DETAIL_HANDLE(destructor)

            STANDARDESE_DETAIL_HANDLE(template_type_parameter)
            STANDARDESE_DETAIL_HANDLE(non_type_template_parameter)
            STANDARDESE_DETAIL_HANDLE(template_template_parameter)

            STANDARDESE_DETAIL_HANDLE(function_template)
            STANDARDESE_DETAIL_HANDLE(function_template_specialization)

            STANDARDESE_DETAIL_HANDLE(class_template)
            STANDARDESE_DETAIL_HANDLE(class_template_full_specialization)
            STANDARDESE_DETAIL_HANDLE(class_template_partial_specialization)

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
