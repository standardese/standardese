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
#include <standardese/parser.hpp>
#include <standardese/translation_unit.hpp>

#include <standardese/detail/synopsis_utils.hpp>

using namespace standardese;

entity_blacklist::entity_blacklist()
{
    type_blacklist_.set(cpp_entity::inclusion_directive_t);
    type_blacklist_.set(cpp_entity::base_class_t);
    type_blacklist_.set(cpp_entity::using_declaration_t);
    type_blacklist_.set(cpp_entity::using_directive_t);
    type_blacklist_.set(cpp_entity::access_specifier_t);
}

namespace
{
    bool is_blacklisted(const std::set<std::pair<cpp_name, cpp_entity::type>> &blacklist, const cpp_entity &e)
    {
        // set is firsted sorted by name, then type
        // so this is the first pair with the name
        static_assert(int(cpp_entity::file_t) == 0, "file must come first");
        auto first = std::make_pair(e.get_name(), cpp_entity::file_t);

        auto iter = blacklist.lower_bound(first);
        if (iter == blacklist.end() || iter->first != e.get_name())
            // no element with this name found
            return false;

        // iterate until name doesn't match any more
        for (; iter != blacklist.end() && iter->first == e.get_name(); ++iter)
            if (iter->second == cpp_entity::invalid_t)
                // match all
                return true;
            else if (iter->second == e.get_entity_type())
                return true;

        return false;
    }
}

bool entity_blacklist::is_blacklisted(documentation_t, const cpp_entity &e) const
{
    if (type_blacklist_[e.get_entity_type()])
        return true;
    return ::is_blacklisted(doc_blacklist_, e);
}

bool entity_blacklist::is_blacklisted(synopsis_t, const cpp_entity &e) const
{
    return ::is_blacklisted(synopsis_blacklist_, e);
}

namespace
{
    void dispatch(const parser &par, output_base::code_block_writer &out, const cpp_entity &e,
                  bool top_level,
                  const cpp_name &override_name = "");

    void do_write_synopsis(const parser &par, output_base::code_block_writer &out,
                           const cpp_file &f)
    {
        detail::write_range(out, f, blankl,
            [&](output_base::code_block_writer &out, const cpp_entity &e)
            {
                if (par.get_output_config().get_blacklist().is_blacklisted(entity_blacklist::synopsis, e))
                    return false;
                dispatch(par, out, e, false);
                return true;
            });
    }

    //=== preprocessor ===//
    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_inclusion_directive &i)
    {
        out << "#include ";

        if (i.get_kind() == cpp_inclusion_directive::local)
            out << '"';
        else
            out << '<';

        out << i.get_file_name();

        if (i.get_kind() == cpp_inclusion_directive::local)
            out << '"';
        else
            out << '>';
    }

    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_macro_definition &m)
    {
        out << "#define " << m.get_name() << m.get_argument_string() << ' ' << m.get_replacement();
    }

    //=== namespace related ===//
    void do_write_synopsis(const parser &par, output_base::code_block_writer &out,
                           const cpp_namespace &ns)
    {
        auto& blacklist = par.get_output_config().get_blacklist();

        if (ns.is_inline())
            out << "inline ";
        out << "namespace " << ns.get_name();

        if (ns.empty())
            out << "{}";
        else
        {
            out << newl << '{' << newl;
            out.indent(par.get_output_config().get_tab_width());

            detail::write_range(out, ns, blankl,
                                [&](output_base::code_block_writer &out, const cpp_entity &e)
                                {
                                    if (blacklist.is_blacklisted(entity_blacklist::synopsis, e))
                                        return false;
                                    dispatch(par, out, e, false);
                                    return true;
                                });

            out.unindent(par.get_output_config().get_tab_width());
            out << newl << '}';
        }
    }

    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_namespace_alias &ns)
    {
        out << "namespace " << ns.get_name() << " = " << ns.get_target().get_name() << ';';
    }

    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_using_directive &u)
    {
        out << "using namespace " << u.get_target().get_name() << ';';
    }

    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_using_declaration &u)
    {
        out << "using " << u.get_target().get_name() << ';';
    }

    //=== types ===//
    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_type_alias &a)
    {
        out << "using " << a.get_name() << " = " << a.get_target().get_name() << ';';
    }

    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_signed_enum_value &e)
    {
        out << e.get_name();
        if (e.is_explicitly_given())
            out << " = " << e.get_value();
    }

    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_unsigned_enum_value &e)
    {
        out << e.get_name();
        if (e.is_explicitly_given())
            out << " = " << e.get_value();
    }

    void do_write_synopsis(const parser &par,
                           output_base::code_block_writer &out,
                           const cpp_enum &e, bool top_level)
    {
        auto& blacklist = par.get_output_config().get_blacklist();

        out << "enum ";
        if (e.is_scoped())
            out << "class ";
        out << e.get_name();
        if (top_level)
        {
            if (!e.get_underlying_type().get_name().empty())
                out << newl << ": " << e.get_underlying_type().get_name();

            if (e.empty())
                out << "{}";
            else
            {
                out << newl << '{' << newl;
                out.indent(par.get_output_config().get_tab_width());

                detail::write_range(out, e, newl, [&](output_base::code_block_writer &out, const cpp_entity &e)
                                    {
                                        if (blacklist.is_blacklisted(entity_blacklist::synopsis, e))
                                            return false;
                                        dispatch(par, out, e, false);
                                        return true;
                                    });

                out.unindent(par.get_output_config().get_tab_width());
                out << newl << '}';
            }
        }
        out << ';';
    }

    void write_access(const parser &par, output_base::code_block_writer &out, cpp_access_specifier_t access)
    {
        out.unindent(par.get_output_config().get_tab_width());
        out << to_string(access) << ':' << newl;
        out.indent(par.get_output_config().get_tab_width());
    }

    void do_write_synopsis(const parser &par,
                           output_base::code_block_writer &out, const cpp_class &c,
                           bool top_level, const cpp_name &override_name)
    {
        detail::write_class_name(out,
                                 override_name.empty() ? c.get_name() : override_name,
                                 c.get_class_type());

        auto& blacklist = par.get_output_config().get_blacklist();
        if (top_level)
        {
            if (c.is_final())
                out << " final";

            detail::write_bases(out, c, blacklist.is_set(entity_blacklist::extract_private));

            // can still be wrongly triggered if a class has only base classes
            if (c.empty())
                out << "{}";
            else
            {
                out << newl << '{' << newl;
                out.indent(par.get_output_config().get_tab_width());

                auto cur_access = c.get_class_type() == cpp_class_t ? cpp_private : cpp_public;
                auto need_access = false;
                auto first = true;
                detail::write_range(out, c, newl,
                    [&](output_base::code_block_writer &out, const cpp_entity &e)
                    {
                        if (e.get_entity_type() == cpp_entity::access_specifier_t)
                        {
                            auto new_access = static_cast<const cpp_access_specifier &>(e).get_access();
                            if (new_access != cur_access)
                                need_access = true;
                            cur_access = new_access;
                        }
                        else if (e.get_entity_type() == cpp_entity::base_class_t
                            || blacklist.is_blacklisted(entity_blacklist::synopsis, e))
                            return false;
                        else if (blacklist.is_set(entity_blacklist::extract_private)
                            || cur_access != cpp_private
                            || detail::is_virtual(e))
                        {
                            if (need_access)
                            {
                                if (!first)
                                   out << blankl;
                                else
                                    first = false;
                                write_access(par, out, cur_access);
                                need_access = false;
                            }
                            dispatch(par, out, e, false);
                            return true;
                        }

                        return false;
                    });

                out.unindent(par.get_output_config().get_tab_width());
                out << newl << '}';
            }
        }

        out << ';';
    }

    //=== variables ===//
    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_variable &v)
    {
        if (v.get_parent().get_entity_type() == cpp_entity::class_t
            || v.get_parent().get_entity_type() == cpp_entity::class_template_t
            || v.get_parent().get_entity_type() == cpp_entity::class_template_full_specialization_t
            || v.get_parent().get_entity_type() == cpp_entity::class_template_partial_specialization_t)
            out << "static ";

        if (v.is_thread_local())
            out << "thread_local ";

        detail::write_type_value_default(out, v.get_type(), v.get_name(), v.get_initializer());
        out << ';';
    }

    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_member_variable &v)
    {
        if (v.is_mutable())
            out << "mutable ";

        detail::write_type_value_default(out, v.get_type(), v.get_name(), v.get_initializer());
        out << ';';
    }

    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_bitfield &v)
    {
        if (v.is_mutable())
            out << "mutable ";

        detail::write_type_value_default(out, v.get_type(), v.get_name());

        out << " : " << (unsigned long long) v.no_bits();

        if (!v.get_initializer().empty())
            out << " = " << v.get_initializer();

        out << ';';
    }

    //=== functions ===//
    void do_write_synopsis(const parser &,
                           output_base::code_block_writer &out, const cpp_function &f,
                           bool, const cpp_name &override_name)
    {
        if (f.is_constexpr())
            out << "constexpr ";

        out << f.get_return_type().get_name() << ' ';
        detail::write_parameters(out, f, override_name);
        detail::write_noexcept(out, f);
        detail::write_definition(out, f);
    }

    void do_write_synopsis(const parser &,
                           output_base::code_block_writer &out, const cpp_member_function &f,
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

    void do_write_synopsis(const parser &,
                           output_base::code_block_writer &out, const cpp_conversion_op &f,
                           bool, const cpp_name &override_name)
    {
        detail::write_prefix(out, f.get_virtual(), f.is_constexpr(), f.is_explicit());
        detail::write_parameters(out, f, override_name);
        detail::write_cv_ref(out, f.get_cv(), f.get_ref_qualifier());
        detail::write_noexcept(out, f);
        detail::write_override_final(out, f.get_virtual());
        detail::write_definition(out, f, f.get_virtual() == cpp_virtual_pure);
    }

    void do_write_synopsis(const parser &,
                           output_base::code_block_writer &out, const cpp_constructor &f,
                           bool, const cpp_name &override_name)
    {
        detail::write_prefix(out, cpp_virtual_none, f.is_constexpr(), f.is_explicit());
        detail::write_parameters(out, f, override_name);
        detail::write_noexcept(out, f);
        detail::write_definition(out, f);
    }

    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_destructor &f,
                           bool, const cpp_name &override_name)
    {
        detail::write_prefix(out, f.get_virtual(), f.is_constexpr());
        detail::write_parameters(out, f, override_name);
        detail::write_noexcept(out, f);
        detail::write_definition(out, f, f.get_virtual() == cpp_virtual_pure);
    }

    //=== templates ===//
    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_template_type_parameter &p)
    {
        out << "typename";
        if (!p.get_name().empty())
            out << ' ' << p.get_name();
        if (p.has_default_type())
            out << " = " << p.get_default_type().get_name();
    }

    void do_write_synopsis(const parser &, output_base::code_block_writer &out, const cpp_non_type_template_parameter &p)
    {
        detail::write_type_value_default(out, p.get_type(), p.get_name(), p.get_default_value());
    }

    void do_write_synopsis(const parser &par,
                           output_base::code_block_writer &out, const cpp_template_template_parameter &p)
    {
        out << "template <";

        detail::write_range(out, p, ", ",
                            [&](output_base::code_block_writer &out, const cpp_entity &e)
                            {
                                dispatch(par, out, e, false);
                                return true;
                            });

        out << "> typename";
        if (!p.get_name().empty())
            out << ' ' << p.get_name();
        if (p.has_default_template())
            out << " = " << p.get_default_template().get_name();
    }

    void do_write_synopsis(const parser &p,
                           output_base::code_block_writer &out, const cpp_function_template &f,
                           bool top_level)
    {
        out << "template <";

        detail::write_range(out, f.get_template_parameters(), ", ",
                            [&](output_base::code_block_writer &out, const cpp_entity &e)
                            {
                                dispatch(p, out, e, false);
                                return true;
                            });

        out << '>' << newl;

        dispatch(p, out, f.get_function(), top_level);
    }

    void do_write_synopsis(const parser &p,
                           output_base::code_block_writer &out, const cpp_function_template_specialization &f,
                           bool top_level)
    {
        out << "template <>" << newl;
        dispatch(p, out, f.get_function(), top_level, f.get_name());
    }

    void do_write_synopsis(const parser &p,
                           output_base::code_block_writer &out, const cpp_class_template &c,
                           bool top_level)
    {
        out << "template <";

        detail::write_range(out, c.get_template_parameters(), ", ",
                            [&](output_base::code_block_writer &out, const cpp_entity &e)
                            {
                                dispatch(p, out, e, false);
                                return true;
                            });

        out << '>' << newl;

        dispatch(p, out, c.get_class(), top_level);
    }

    void do_write_synopsis(const parser &p,
                           output_base::code_block_writer &out, const cpp_class_template_full_specialization &c,
                           bool top_level)
    {
        out << "template <>" << newl;
        dispatch(p, out, c.get_class(), top_level, c.get_name());
    }

    void do_write_synopsis(const parser &p,
                           output_base::code_block_writer &out, const cpp_class_template_partial_specialization &c,
                           bool top_level)
    {
        out << "template <";

        detail::write_range(out, c.get_template_parameters(), ", ",
                            [&](output_base::code_block_writer &out, const cpp_entity &e)
                            {
                                dispatch(p, out, e, false);
                                return true;
                            });

        out << '>' << newl;

        dispatch(p, out, c.get_class(), top_level, c.get_name());
    }

    //=== dispatching ===//
    template <typename T>
    void do_write_synopsis(const parser &p, output_base::code_block_writer &out,
                           const T &e, bool)
    {
        do_write_synopsis(p, out, e);
    }

    template <typename T>
    void do_write_synopsis(const parser &p, output_base::code_block_writer &out,
                           const T &e,
                           bool top_level, const cpp_name &)
    {
        do_write_synopsis(p, out, e, top_level);
    }

    void dispatch(const parser &p, output_base::code_block_writer &out, const cpp_entity &e,
                  bool top_level,
                  const cpp_name &override_name)
    {
        switch (e.get_entity_type())
        {
            #define STANDARDESE_DETAIL_HANDLE(name) \
        case cpp_entity::name##_t: \
            do_write_synopsis(p, out, static_cast<const cpp_##name&>(e), top_level, override_name); \
            break;

            STANDARDESE_DETAIL_HANDLE(file)

            STANDARDESE_DETAIL_HANDLE(inclusion_directive)
            STANDARDESE_DETAIL_HANDLE(macro_definition)

            STANDARDESE_DETAIL_HANDLE(namespace)
            STANDARDESE_DETAIL_HANDLE(namespace_alias)
            STANDARDESE_DETAIL_HANDLE(using_directive)
            STANDARDESE_DETAIL_HANDLE(using_declaration)

            STANDARDESE_DETAIL_HANDLE(type_alias)

            STANDARDESE_DETAIL_HANDLE(signed_enum_value)
            STANDARDESE_DETAIL_HANDLE(unsigned_enum_value)
            STANDARDESE_DETAIL_HANDLE(enum)

            STANDARDESE_DETAIL_HANDLE(class)

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

void standardese::write_synopsis(const parser &p, output_base &out, const cpp_entity &e)
{
    output_base::code_block_writer w(out);
    dispatch(p, w, e, true);
}
