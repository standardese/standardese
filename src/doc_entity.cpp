// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/doc_entity.hpp>

#include <standardese/detail/synopsis_utils.hpp>
#include <standardese/cpp_class.hpp>
#include <standardese/cpp_enum.hpp>
#include <standardese/cpp_function.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/cpp_variable.hpp>
#include <standardese/index.hpp>
#include <standardese/md_blocks.hpp>
#include <standardese/md_custom.hpp>
#include <standardese/md_inlines.hpp>
#include <standardese/output.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

void detail::generation_access::do_generate_synopsis(const doc_entity& e, const parser& p,
                                                     code_block_writer& out, bool top_level)
{
    e.do_generate_synopsis(p, out, top_level);
}

void detail::generation_access::do_generate_documentation_inline(const doc_entity& e,
                                                                 const parser& p, const index& i,
                                                                 md_inline_documentation& doc)
{
    e.do_generate_documentation_inline(p, i, doc);
}

bool doc_entity::in_module() const STANDARDESE_NOEXCEPT
{
    return !get_module().empty();
}

const std::string& doc_entity::get_module() const STANDARDESE_NOEXCEPT
{
    static std::string empty;
    if (has_comment() && get_comment().in_module())
        return get_comment().get_module();
    return has_parent() ? get_parent().get_module() : empty;
}

cpp_name doc_entity::get_unique_name() const
{
    return detail::get_unique_name(has_parent() ? &get_parent() : nullptr, do_get_unique_name(),
                                   comment_);
}

doc_entity::doc_entity(doc_entity::type t, const doc_entity* parent,
                       const comment* c) STANDARDESE_NOEXCEPT : parent_(parent),
                                                                comment_(c),
                                                                t_(t)
{
    if (comment_)
        comment_->get_content().set_entity(*this);
}

namespace
{
    const char* get_entity_type_spelling(const cpp_entity& e)
    {
        switch (e.get_entity_type())
        {
        case cpp_entity::file_t:
            return "header file";

        case cpp_entity::inclusion_directive_t:
            return "inclusion directive";
        case cpp_entity::macro_definition_t:
            return "macro";

        case cpp_entity::language_linkage_t:
            return "language linkage";
        case cpp_entity::namespace_t:
            return "namespace";
        case cpp_entity::namespace_alias_t:
            return "namespace alias";
        case cpp_entity::using_directive_t:
            return "using directive";
        case cpp_entity::using_declaration_t:
            return "using declaration";

        case cpp_entity::type_alias_t:
            return "type alias";
        case cpp_entity::alias_template_t:
            return "alias template";

        case cpp_entity::enum_t:
            return "enumeration";
        case cpp_entity::signed_enum_value_t:
        case cpp_entity::unsigned_enum_value_t:
        case cpp_entity::expression_enum_value_t:
            return "enumeration constant";

        case cpp_entity::variable_t:
        case cpp_entity::member_variable_t:
        case cpp_entity::bitfield_t:
            return "variable";

        case cpp_entity::function_parameter_t:
            return "parameter";
        case cpp_entity::function_t:
        case cpp_entity::member_function_t:
        {
            auto& func = static_cast<const cpp_function_base&>(e);
            switch (func.get_operator_kind())
            {
            case cpp_operator_none:
                break;
            case cpp_operator:
                return "operator";

            case cpp_assignment_operator:
                return "assignment operator";
            case cpp_copy_assignment_operator:
                return "copy assignment operator";
            case cpp_move_assignment_operator:
                return "move assignment operator";

            case cpp_comparison_operator:
                return "comparison operator";
            case cpp_subscript_operator:
                return "array subscript operator";
            case cpp_function_call_operator:
                return "function call operator";
            case cpp_user_defined_literal:
                return "user defined literal";

            case cpp_output_operator:
                return "output operator";
            case cpp_input_operator:
                return "input operator";
            }
            return "function";
        }
        case cpp_entity::conversion_op_t:
            return "conversion operator";
        case cpp_entity::constructor_t:
        {
            auto& ctor = static_cast<const cpp_constructor&>(e);
            switch (ctor.get_ctor_type())
            {
            case cpp_default_ctor:
                return "default constructor";
            case cpp_copy_ctor:
                return "copy constructor";
            case cpp_move_ctor:
                return "move constructor";
            case cpp_other_ctor:
                break;
            }
            return "constructor";
        }
        case cpp_entity::destructor_t:
            return "destructor";

        case cpp_entity::template_type_parameter_t:
        case cpp_entity::non_type_template_parameter_t:
        case cpp_entity::template_template_parameter_t:
            return "template parameter";

        case cpp_entity::function_template_t:
        case cpp_entity::function_template_specialization_t:
            return "function template";

        case cpp_entity::class_t:
        {
            switch (static_cast<const cpp_class&>(e).get_class_type())
            {
            case cpp_class_t:
                return "class";
            case cpp_struct_t:
                return "struct";
            case cpp_union_t:
                return "union";
            }
            assert(false);
            return "weird class type - should not get here";
        }
        case cpp_entity::class_template_t:
        case cpp_entity::class_template_full_specialization_t:
        case cpp_entity::class_template_partial_specialization_t:
            return "class template";

        case cpp_entity::base_class_t:
            return "base class";
        case cpp_entity::access_specifier_t:
            return "access specifier";

        case cpp_entity::invalid_t:
            break;
        }

        assert(false);
        return "should never get here";
    }

    using standardese::index;

    md_ptr<md_heading> make_heading(const index& i, const doc_cpp_entity& e,
                                    const md_entity& parent, unsigned level, bool show_module)
    {
        auto heading = md_heading::make(parent, level);

        auto type     = get_entity_type_spelling(e.get_cpp_entity());
        auto text_str = fmt::format("{}{} ", char(std::toupper(type[0])), &type[1]);
        auto text     = md_text::make(*heading, text_str.c_str());
        heading->add_entity(std::move(text));

        auto code = md_code::make(*heading, e.get_index_name(true, false).c_str());
        heading->add_entity(std::move(code));

        if (show_module && e.has_comment() && e.get_comment().in_module())
            heading->add_entity(
                md_text::make(*heading,
                              fmt::format(" [{}]", e.get_comment().get_module()).c_str()));

        heading->add_entity(i.get_linker().get_anchor(e, *heading));

        return heading;
    }

    bool has_comment_impl(const doc_entity& e)
    {
        auto c = e.has_comment() ? &e.get_comment() : nullptr;
        return c && (!c->empty() || c->in_member_group());
    }

    bool requires_comment_for_doc(const doc_entity& e)
    {
        return e.get_cpp_entity_type() != cpp_entity::file_t;
    }
}

bool doc_cpp_entity::do_generate_documentation_base(const parser& p, const index& i,
                                                    md_document& doc, unsigned level) const
{
    if ((requires_comment_for_doc(*this) && !has_comment_impl(*this))
        || p.get_output_config().get_blacklist().is_blacklisted(entity_blacklist::documentation,
                                                                get_cpp_entity()))
        return false;

    doc.add_entity(make_heading(i, *this, doc, level,
                                p.get_output_config().is_set(output_flag::show_modules)));

    code_block_writer out(doc, p.get_output_config().is_set(output_flag::use_advanced_code_block));
    do_generate_synopsis(p, out, true);
    doc.add_entity(out.get_code_block());

    if (has_comment())
        doc.add_entity(get_comment().get_content().clone(doc));

    return true;
}

cpp_name doc_cpp_entity::do_get_unique_name() const
{
    return entity_->get_unique_name(true);
}

cpp_name doc_cpp_entity::do_get_index_name(bool full_name, bool signature) const
{
    if (get_cpp_entity_type() == cpp_entity::namespace_t)
        return entity_->get_full_name();
    auto result =
        std::string(full_name ? entity_->get_full_name().c_str() : entity_->get_name().c_str());
    if (!signature)
        return result;
    else if (auto func = get_function(*entity_))
        result += func->get_signature().c_str();
    return result;
}

bool standardese::is_inline_cpp_entity(cpp_entity::type t) STANDARDESE_NOEXCEPT
{
    return t == cpp_entity::base_class_t || is_parameter(t) || is_member_variable(t)
           || is_enum_value(t);
}

void doc_inline_cpp_entity::do_generate_documentation(const parser& p, const index& i,
                                                      md_document& doc, unsigned level) const
{
    assert(!p.get_output_config().is_set(output_flag::inline_documentation));

    do_generate_documentation_base(p, i, doc, level);
}

void doc_inline_cpp_entity::do_generate_documentation_inline(const parser& p, const index& i,
                                                             md_inline_documentation& doc) const
{
    assert(p.get_output_config().is_set(output_flag::inline_documentation));
    if (has_comment())
        doc.add_item(get_name().c_str(), i.get_linker().get_anchor_id(*this).c_str(),
                     get_comment().get_content());
}

namespace
{
    template <class EnumValue>
    void do_write_synopsis_enum_value(code_block_writer& out, bool top_level,
                                      const doc_cpp_entity& e, const EnumValue& val)
    {
        out.write_link(top_level, val.get_name(), e.get_unique_name());
        if (val.is_explicitly_given())
            out << " = " << val.get_value();
    }

    void do_write_synopsis_member_variable(const parser& p, code_block_writer& out, bool top_level,
                                           const cpp_member_variable_base& v)
    {
        if (v.is_mutable())
            out << "mutable ";

        detail::write_type_value_default(p, out, top_level, v.get_type(), v.get_name(),
                                         v.get_unique_name());

        if (v.get_entity_type() == cpp_entity::bitfield_t)
            out << " : "
                << static_cast<unsigned long long>(static_cast<const cpp_bitfield&>(v).no_bits());

        if (!v.get_initializer().empty())
            out << " = " << v.get_initializer();

        out << ';';
    }

    void do_write_synopsis_template_param(const parser& p, code_block_writer& out,
                                          const cpp_template_type_parameter& param)
    {
        if (param.get_keyword() == cpp_template_parameter::cpp_typename)
            out << "typename";
        else if (param.get_keyword() == cpp_template_parameter::cpp_class)
            out << "class";
        else
            assert(false);

        if (param.is_variadic())
            out << " ...";

        if (!param.get_name().empty())
            out << ' ' << param.get_name();

        if (param.has_default_type())
        {
            auto def_name = detail::get_ref_name(p, param.get_default_type());
            out << " = ";
            detail::write_type_ref_name(out, def_name);
        }
    }

    void do_write_synopsis_template_param(const parser& p, code_block_writer& out,
                                          const cpp_non_type_template_parameter& param)
    {
        detail::write_type_value_default(p, out, false, param.get_type(), param.get_name(), "",
                                         param.get_default_value(), param.is_variadic());
    }

    void do_write_synopsis_template_param(const parser& p, code_block_writer& out,
                                          const cpp_template_parameter& param);

    void do_write_synopsis_template_param(const parser& p, code_block_writer& out,
                                          const cpp_template_template_parameter& param)
    {
        out << "template <";

        auto first = true;
        for (auto& child : param)
        {
            if (first)
                first = false;
            else
                out << ", ";

            do_write_synopsis_template_param(p, out, child);
        }

        out << "> ";

        if (param.get_keyword() == cpp_template_parameter::cpp_typename)
            out << "typename";
        else if (param.get_keyword() == cpp_template_parameter::cpp_class)
            out << "class";
        else
            assert(false);

        if (param.is_variadic())
            out << " ...";
        if (!param.get_name().empty())
            out << ' ' << param.get_name();
        if (param.has_default_template())
        {
            auto def = detail::get_ref_name(p, param.get_default_template());
            (out << " = ").write_link(false, def.name, def.unique_name);
        }
    }

    void do_write_synopsis_template_param(const parser& p, code_block_writer& out,
                                          const cpp_template_parameter& param)
    {
        if (param.get_entity_type() == cpp_entity::template_type_parameter_t)
            do_write_synopsis_template_param(p, out,
                                             static_cast<const cpp_template_type_parameter&>(
                                                 param));
        else if (param.get_entity_type() == cpp_entity::non_type_template_parameter_t)
            do_write_synopsis_template_param(p, out,
                                             static_cast<const cpp_non_type_template_parameter&>(
                                                 param));
        else if (param.get_entity_type() == cpp_entity::template_template_parameter_t)
            do_write_synopsis_template_param(p, out,
                                             static_cast<const cpp_template_template_parameter&>(
                                                 param));
        else
            assert(false);
    }
}

void doc_inline_cpp_entity::do_generate_synopsis(const parser& p, code_block_writer& out,
                                                 bool top_level) const
{
    if (has_comment() && get_comment().has_synopsis_override())
    {
        out << get_comment().get_synopsis_override();
        return;
    }

    switch (get_cpp_entity_type())
    {
    case cpp_entity::signed_enum_value_t:
        do_write_synopsis_enum_value(out, top_level, *this,
                                     static_cast<const cpp_signed_enum_value&>(get_cpp_entity()));
        break;
    case cpp_entity::unsigned_enum_value_t:
        do_write_synopsis_enum_value(out, top_level, *this,
                                     static_cast<const cpp_unsigned_enum_value&>(get_cpp_entity()));
        break;
    case cpp_entity::expression_enum_value_t:
        do_write_synopsis_enum_value(out, top_level, *this,
                                     static_cast<const cpp_expression_enum_value&>(
                                         get_cpp_entity()));
        break;

    case cpp_entity::function_parameter_t:
    {
        auto& param = static_cast<const cpp_function_parameter&>(get_cpp_entity());
        detail::write_type_value_default(p, out, top_level, param.get_type(), param.get_name(),
                                         param.get_default_value());
        break;
    }

    case cpp_entity::member_variable_t:
    case cpp_entity::bitfield_t:
        do_write_synopsis_member_variable(p, out, top_level,
                                          static_cast<const cpp_member_variable_base&>(
                                              get_cpp_entity()));
        break;

    case cpp_entity::template_type_parameter_t:
    case cpp_entity::non_type_template_parameter_t:
    case cpp_entity::template_template_parameter_t:
        do_write_synopsis_template_param(p, out, static_cast<const cpp_template_parameter&>(
                                                     get_cpp_entity()));
        break;

    case cpp_entity::base_class_t:
    {
        auto& base      = static_cast<const cpp_base_class&>(get_cpp_entity());
        auto  base_name = detail::get_ref_name(p, base.get_type());
        (out << to_string(base.get_access()) << ' ')
            .write_link(top_level, base_name.name, base_name.unique_name);
    }

    case cpp_entity::file_t:
    case cpp_entity::inclusion_directive_t:
    case cpp_entity::macro_definition_t:
    case cpp_entity::language_linkage_t:
    case cpp_entity::namespace_t:
    case cpp_entity::namespace_alias_t:
    case cpp_entity::using_directive_t:
    case cpp_entity::using_declaration_t:
    case cpp_entity::type_alias_t:
    case cpp_entity::alias_template_t:
    case cpp_entity::enum_t:
    case cpp_entity::variable_t:
    case cpp_entity::function_t:
    case cpp_entity::member_function_t:
    case cpp_entity::conversion_op_t:
    case cpp_entity::constructor_t:
    case cpp_entity::destructor_t:
    case cpp_entity::function_template_t:
    case cpp_entity::function_template_specialization_t:
    case cpp_entity::class_t:
    case cpp_entity::class_template_t:
    case cpp_entity::class_template_partial_specialization_t:
    case cpp_entity::class_template_full_specialization_t:
    case cpp_entity::access_specifier_t:
    case cpp_entity::invalid_t:
        assert(false);
        break;
    }
}

doc_inline_cpp_entity::doc_inline_cpp_entity(const doc_entity* parent, const cpp_entity& e,
                                             const comment* c)
: doc_cpp_entity(parent, e, c)
{
    assert(is_inline_cpp_entity(e.get_entity_type()));
}

void doc_cpp_access_entity::do_generate_synopsis(const parser& p, code_block_writer& out,
                                                 bool) const
{
    out.unindent(p.get_output_config().get_tab_width());
    out << to_string(static_cast<const cpp_access_specifier&>(get_cpp_entity()).get_access())
        << ':';
    out.indent(p.get_output_config().get_tab_width());
}

doc_cpp_access_entity::doc_cpp_access_entity(const doc_entity* parent, const cpp_entity& e,
                                             const comment* c)
: doc_cpp_entity(parent, e, c)
{
}

void doc_leave_cpp_entity::do_generate_documentation(const parser& p, const index& i,
                                                     md_document& doc, unsigned level) const
{
    do_generate_documentation_base(p, i, doc, level);
}

namespace
{
    void do_write_synopsis(const parser& par, code_block_writer& out, bool top_level,
                           const cpp_namespace_alias& ns, const comment* c)
    {
        (out << "namespace ").write_link(top_level, ns.get_name(), ns.get_unique_name()) << " = ";
        if (c && c->get_excluded() == exclude_mode::target)
            out << par.get_output_config().get_hidden_name();
        else
        {
            auto target = detail::get_ref_name(par, ns.get_target());
            out.write_link(false, target.name, target.unique_name);
        }
        out << ';';
    }

    void do_write_synopsis(const parser& par, code_block_writer& out, const cpp_using_directive& u)
    {
        auto target = detail::get_ref_name(par, u.get_target());
        (out << "using namespace ").write_link(false, target.name, target.unique_name) << ';';
    }

    void do_write_synopsis(const parser& par, code_block_writer& out,
                           const cpp_using_declaration& u)
    {
        auto target = detail::get_ref_name(par, u.get_target());
        (out << "using ").write_link(false, target.name, target.unique_name) << ';';
    }

    void do_write_synopsis(const parser& par, code_block_writer& out, bool top_level,
                           const doc_cpp_entity& e, const cpp_type_alias& a)
    {
        (out << "using ").write_link(top_level, a.get_name(), e.get_unique_name()) << " = ";
        if (e.has_comment() && e.get_comment().get_excluded() == exclude_mode::target)
            out << par.get_output_config().get_hidden_name() << ';';
        else
        {
            auto target = detail::get_ref_name(par, a.get_target());
            detail::write_type_ref_name(out, target);
            out << ';';
        }
    }

    void do_write_synopsis(const parser& par, code_block_writer& out, bool top_level,
                           const doc_cpp_entity& e, const cpp_variable& v)
    {
        if (v.get_ast_parent().get_entity_type() == cpp_entity::class_t)
            out << "static ";

        if (v.is_thread_local())
            out << "thread_local ";

        detail::write_type_value_default(par, out, top_level, v.get_type(), v.get_name(),
                                         e.get_unique_name(), v.get_initializer());
        out << ';';
    }

    void do_write_synopsis(const parser&, code_block_writer& out, const cpp_inclusion_directive& i)
    {
        out << "#include ";
        out << (i.get_kind() == cpp_inclusion_directive::system ? '<' : '"');
        out << i.get_file_name();
        out << (i.get_kind() == cpp_inclusion_directive::system ? '>' : '"');
    }

    void do_write_synopsis(const parser&, code_block_writer& out, bool top_level,
                           const doc_cpp_entity& e, const cpp_macro_definition& m,
                           bool show_replacement)
    {
        (out << "#define ").write_link(top_level, m.get_name(), e.get_unique_name())
            << m.get_parameter_string();
        if (show_replacement)
            out << ' ' << m.get_replacement();
    }
}

void doc_leave_cpp_entity::do_generate_synopsis(const parser& p, code_block_writer& out,
                                                bool top_level) const
{
    if (has_comment() && get_comment().has_synopsis_override())
    {
        out << get_comment().get_synopsis_override();
        return;
    }

    switch (get_cpp_entity_type())
    {
    case cpp_entity::macro_definition_t:
        do_write_synopsis(p, out, top_level, *this,
                          static_cast<const cpp_macro_definition&>(get_cpp_entity()),
                          p.get_output_config().is_set(output_flag::show_macro_replacement));
        break;
    case cpp_entity::inclusion_directive_t:
        do_write_synopsis(p, out, static_cast<const cpp_inclusion_directive&>(get_cpp_entity()));
        break;

    case cpp_entity::namespace_alias_t:
        do_write_synopsis(p, out, top_level,
                          static_cast<const cpp_namespace_alias&>(get_cpp_entity()),
                          has_comment() ? &get_comment() : nullptr);
        break;
    case cpp_entity::using_directive_t:
        do_write_synopsis(p, out, static_cast<const cpp_using_directive&>(get_cpp_entity()));
        break;
    case cpp_entity::using_declaration_t:
        do_write_synopsis(p, out, static_cast<const cpp_using_declaration&>(get_cpp_entity()));
        break;

    case cpp_entity::type_alias_t:
        do_write_synopsis(p, out, top_level, *this,
                          static_cast<const cpp_type_alias&>(get_cpp_entity()));
        break;

    case cpp_entity::variable_t:
        do_write_synopsis(p, out, top_level, *this,
                          static_cast<const cpp_variable&>(get_cpp_entity()));
        break;

    case cpp_entity::file_t:
    case cpp_entity::language_linkage_t:
    case cpp_entity::namespace_t:
    case cpp_entity::alias_template_t:
    case cpp_entity::enum_t:
    case cpp_entity::signed_enum_value_t:
    case cpp_entity::unsigned_enum_value_t:
    case cpp_entity::expression_enum_value_t:
    case cpp_entity::member_variable_t:
    case cpp_entity::bitfield_t:
    case cpp_entity::function_parameter_t:
    case cpp_entity::function_t:
    case cpp_entity::member_function_t:
    case cpp_entity::conversion_op_t:
    case cpp_entity::constructor_t:
    case cpp_entity::destructor_t:
    case cpp_entity::template_type_parameter_t:
    case cpp_entity::non_type_template_parameter_t:
    case cpp_entity::template_template_parameter_t:
    case cpp_entity::function_template_t:
    case cpp_entity::function_template_specialization_t:
    case cpp_entity::class_t:
    case cpp_entity::class_template_t:
    case cpp_entity::class_template_partial_specialization_t:
    case cpp_entity::class_template_full_specialization_t:
    case cpp_entity::base_class_t:
    case cpp_entity::access_specifier_t:
    case cpp_entity::invalid_t:
        assert(false);
        break;
    }
}

namespace
{
    bool is_leave_entity(const cpp_entity& e)
    {
        return e.get_entity_type() == cpp_entity::namespace_alias_t
               || e.get_entity_type() == cpp_entity::using_declaration_t
               || e.get_entity_type() == cpp_entity::using_directive_t
               || e.get_entity_type() == cpp_entity::type_alias_t
               || e.get_entity_type() == cpp_entity::variable_t
               || is_preprocessor(e.get_entity_type());
    }
}

doc_leave_cpp_entity::doc_leave_cpp_entity(const doc_entity* parent, const cpp_entity& e,
                                           const comment* c)
: doc_cpp_entity(parent, e, c)
{
    assert(is_leave_entity(e));
}

namespace
{
    struct inline_container
    {
        md_ptr<md_inline_documentation> parameters, bases, members, enum_values;

        inline_container(md_document& doc)
        : parameters(md_inline_documentation::make(doc, "Parameters")),
          bases(md_inline_documentation::make(doc, "Bases")),
          members(md_inline_documentation::make(doc, "Members")),
          enum_values(md_inline_documentation::make(doc, "Enum values"))
        {
        }

        void add_inlines(const parser& p, const index& i, const doc_entity& container)
        {
            for (auto& child : container)
            {
                if (is_parameter(child.get_cpp_entity_type()))
                    detail::generation_access::do_generate_documentation_inline(child, p, i,
                                                                                *parameters);
                else if (child.get_cpp_entity_type() == cpp_entity::base_class_t)
                    detail::generation_access::do_generate_documentation_inline(child, p, i,
                                                                                *bases);
                else if (is_member_variable(child.get_cpp_entity_type()))
                    detail::generation_access::do_generate_documentation_inline(child, p, i,
                                                                                *members);
                else if (is_enum_value(child.get_cpp_entity_type()))
                    detail::generation_access::do_generate_documentation_inline(child, p, i,
                                                                                *enum_values);
                else
                    assert(!is_inline_cpp_entity(child.get_cpp_entity_type()));
            }
        }

        void finish(md_document& doc)
        {
            if (!parameters->empty())
                doc.add_entity(std::move(parameters));
            if (!bases->empty())
                doc.add_entity(std::move(bases));
            if (!members->empty())
                doc.add_entity(std::move(members));
            if (!enum_values->empty())
                doc.add_entity(std::move(enum_values));
        }
    };

    bool requires_comment(const doc_entity& e)
    {
        return requires_comment_for_doc(e) && e.get_cpp_entity_type() != cpp_entity::namespace_t
               && e.get_cpp_entity_type() != cpp_entity::language_linkage_t;
    }
}

void doc_container_cpp_entity::do_generate_documentation(const parser& p, const index& i,
                                                         md_document& doc, unsigned level) const
{
    auto generate_doc = do_generate_documentation_base(p, i, doc, level);
    if (generate_doc)
    {
        // add inline entity comments
        if (p.get_output_config().is_set(output_flag::inline_documentation))
        {
            inline_container inlines(doc);
            inlines.add_inlines(p, i, *this);
            inlines.finish(doc);
        }
    }

    // add documentation for other children
    auto any_child = false;
    for (auto& child : *this)
    {
        if (p.get_output_config().is_set(output_flag::inline_documentation)
            && is_inline_cpp_entity(child.get_cpp_entity_type()))
            continue;
        else if (!requires_comment(child) || has_comment_impl(child))
        {
            child.do_generate_documentation(p, i, doc, generate_doc ? level + 1 : level);
            any_child = true;
        }
    }

    if (any_child && get_cpp_entity_type() != cpp_entity::file_t)
        doc.add_entity(md_thematic_break::make(doc));
}

namespace
{
    void do_write_synopsis(const parser& p, code_block_writer& out, bool top_level,
                           const doc_container_cpp_entity& e, const cpp_class& c)
    {
        if (e.get_cpp_entity_type() == cpp_entity::class_template_full_specialization_t
            || e.get_cpp_entity_type() == cpp_entity::class_template_partial_specialization_t)
            detail::write_class_name(out, top_level, e.get_cpp_entity().get_name(),
                                     e.get_unique_name(), c.get_class_type());
        else if (e.get_cpp_entity_type() == cpp_entity::class_template_t)
            detail::write_class_name(out, top_level, e.get_cpp_entity().get_name(),
                                     e.get_unique_name(), c.get_class_type());
        else
            detail::write_class_name(out, top_level, c.get_name(), e.get_unique_name(),
                                     c.get_class_type());

        if (e.get_cpp_entity().get_name().empty() || top_level)
        {
            if (c.is_final())
                out << " final";

            out << newl;
            detail::write_bases(p, out, e, c);
            out << '{' << newl;
            out.indent(p.get_output_config().get_tab_width());

            auto first = true;
            auto last_access =
                c.get_class_type() == cpp_class_t ? cpp_private : cpp_public; // last written access
            auto cur_access  = last_access; // access of current entity
            auto need_access = false;       // need change in access modifier
            for (auto& child : e)
            {
                if (child.get_cpp_entity_type() == cpp_entity::base_class_t
                    || is_template_parameter(child.get_cpp_entity_type()))
                    continue;
                else if (detail::is_blacklisted(p, child))
                    continue;

                if (child.get_cpp_entity_type() == cpp_entity::access_specifier_t)
                {
                    cur_access = static_cast<const cpp_access_specifier&>(
                                     static_cast<const doc_cpp_entity&>(child).get_cpp_entity())
                                     .get_access();

                    if (cur_access != last_access)
                        // change in access
                        need_access = true;
                    else if (need_access)
                        // no change but change still requested
                        // because a previous entity changed it but it wasn't written after all
                        need_access = false;
                }
                else
                {
                    if (first)
                        first = false;
                    else
                        out << blankl;

                    if (need_access)
                    {
                        out.unindent(p.get_output_config().get_tab_width());
                        out << to_string(cur_access) << ':' << newl;
                        out.indent(p.get_output_config().get_tab_width());

                        need_access = false;
                        last_access = cur_access;
                    }

                    detail::generation_access::do_generate_synopsis(child, p, out, false);
                }
            }
            out.remove_trailing_line();
            out.unindent(p.get_output_config().get_tab_width());
            out << newl << '}';
        }
        out << ';';
    }

    void do_write_synopsis(const parser& p, code_block_writer& out, bool top_level,
                           const doc_container_cpp_entity& e, const cpp_function_base& f,
                           const comment* c)
    {
        detail::write_prefix(out, detail::get_virtual(f), f.is_constexpr());

        if (c && c->has_return_type_override())
            out << c->get_synopsis_override() << ' ';
        else if (c && c->get_excluded() == exclude_mode::return_type)
            out << p.get_output_config().get_hidden_name() << ' ';
        else if (f.get_entity_type() == cpp_entity::function_t)
        {
            auto ret_name =
                detail::get_ref_name(p, static_cast<const cpp_function&>(f).get_return_type());
            detail::write_type_ref_name(out, ret_name);
            out << ' ';
        }
        else if (f.get_entity_type() == cpp_entity::member_function_t)
        {
            auto ret_name =
                detail::get_ref_name(p,
                                     static_cast<const cpp_member_function&>(f).get_return_type());
            detail::write_type_ref_name(out, ret_name);
            out << ' ';
        }

        detail::write_parameters(p, out, top_level, e, f);

        detail::write_cv_ref(out, detail::get_cv(f), detail::get_ref_qualifier(f));
        detail::write_noexcept(p.get_output_config().is_set(output_flag::show_complex_noexcept) ?
                                   nullptr :
                                   p.get_output_config().get_hidden_name().c_str(),
                               out, f);

        detail::write_override_final(out, detail::get_virtual(f));

        detail::write_definition(out, f);
    }

    template <typename Seperator>
    void do_write_synopsis_container(const parser& p, code_block_writer& out,
                                     const doc_container_cpp_entity& e, Seperator sep)
    {
        out << '{' << newl;
        out.indent(p.get_output_config().get_tab_width());
        auto first = true;
        for (auto& child : e)
        {
            if (detail::is_blacklisted(p, child))
                continue;

            if (first)
                first = false;
            else
                out << sep;

            detail::generation_access::do_generate_synopsis(child, p, out, false);
        }
        out.remove_trailing_line();
        out.unindent(p.get_output_config().get_tab_width());
        out << newl << '}';
    }

    void do_write_synopsis(const parser& p, code_block_writer& out, bool top_level,
                           const doc_container_cpp_entity& entity, const cpp_enum& e)
    {
        out << "enum ";
        if (e.is_scoped())
            out << "class ";
        out.write_link(top_level, entity.get_cpp_entity().get_name(), entity.get_unique_name());
        if (entity.get_cpp_entity().get_name().empty() || top_level)
        {
            if (!e.get_underlying_type().get_name().empty())
            {
                out << newl << ": ";
                if (entity.has_comment()
                    && entity.get_comment().get_excluded() == exclude_mode::target)
                    out << p.get_output_config().get_hidden_name();
                else
                {
                    auto type_name = detail::get_ref_name(p, e.get_underlying_type());
                    out.write_link(false, type_name.name, type_name.unique_name);
                }
            }

            out << newl;
            do_write_synopsis_container(p, out, entity, ",\n");
        }
        out << ';';
    }

    void do_write_synopsis(const parser& p, code_block_writer& out, bool top_level,
                           const doc_container_cpp_entity& entity, const cpp_namespace& ns)
    {
        if (ns.is_inline())
            out << "inline ";
        (out << "namespace ").write_link(top_level, ns.get_name(), entity.get_unique_name())
            << newl;
        do_write_synopsis_container(p, out, entity, blankl);
    }

    void do_write_synopsis(const parser& p, code_block_writer& out,
                           const doc_container_cpp_entity& entity, const cpp_language_linkage& lang)
    {
        out << "extern \"" << lang.get_name() << '"' << newl;
        do_write_synopsis_container(p, out, entity, blankl);
    }
}

void doc_container_cpp_entity::do_generate_synopsis(const parser& p, code_block_writer& out,
                                                    bool top_level) const
{
    if (has_comment() && get_comment().has_synopsis_override())
    {
        out << get_comment().get_synopsis_override();
        return;
    }

    // write template parameters, if there are any
    if (is_template(get_cpp_entity_type()))
        detail::write_template_parameters(p, out, *this);

    if (auto c = get_class(get_cpp_entity()))
        do_write_synopsis(p, out, top_level, *this, *c);
    else if (auto func = get_function(get_cpp_entity()))
        do_write_synopsis(p, out, top_level, *this, *func,
                          has_comment() ? &get_comment() : nullptr);
    else if (get_cpp_entity_type() == cpp_entity::enum_t)
        do_write_synopsis(p, out, top_level, *this, static_cast<const cpp_enum&>(get_cpp_entity()));
    else if (get_cpp_entity_type() == cpp_entity::alias_template_t)
        do_write_synopsis(p, out, top_level, *this,
                          static_cast<const cpp_alias_template&>(get_cpp_entity()).get_type());
    else if (get_cpp_entity_type() == cpp_entity::namespace_t)
        do_write_synopsis(p, out, top_level, *this,
                          static_cast<const cpp_namespace&>(get_cpp_entity()));
    else if (get_cpp_entity_type() == cpp_entity::language_linkage_t)
        do_write_synopsis(p, out, *this,
                          static_cast<const cpp_language_linkage&>(get_cpp_entity()));
    else if (get_cpp_entity_type() == cpp_entity::file_t)
    {
        auto first = true;
        for (auto& child : *this)
        {
            if (detail::is_blacklisted(p, child))
                continue;

            if (first)
                first = false;
            else
                out << blankl;

            detail::generation_access::do_generate_synopsis(child, p, out, false);
        }
        out.remove_trailing_line();
    }
    else
        assert(false);
}

namespace
{
    bool is_container_entity(const cpp_entity& e)
    {
        return e.get_entity_type() == cpp_entity::namespace_t
               || e.get_entity_type() == cpp_entity::language_linkage_t
               || e.get_entity_type() == cpp_entity::enum_t
               || e.get_entity_type() == cpp_entity::class_t
               || e.get_entity_type() == cpp_entity::file_t || is_function_like(e.get_entity_type())
               || is_template(e.get_entity_type());
    }
}

doc_container_cpp_entity::doc_container_cpp_entity(const doc_entity* parent, const cpp_entity& e,
                                                   const comment* c)
: doc_cpp_entity(parent, e, c)
{
    assert(is_container_entity(e));
}

doc_ptr<doc_member_group> doc_member_group::make(const doc_entity& parent, const comment& c)
{
    return detail::make_doc_ptr<doc_member_group>(&parent, c);
}

std::size_t doc_member_group::group_id() const STANDARDESE_NOEXCEPT
{
    return get_comment().member_group_id();
}

void doc_member_group::do_generate_documentation(const parser& p, const index& i, md_document& doc,
                                                 unsigned level) const
{
    assert(!empty());

    if (get_comment().has_group_name())
    {
        auto heading = md_heading::make(doc, level);
        heading->add_entity(md_text::make(*heading, get_comment().get_group_name().c_str()));
        heading->add_entity(i.get_linker().get_anchor(*begin(), *heading));
        doc.add_entity(std::move(heading));
    }
    else if (begin()->get_entity_type() == doc_entity::cpp_entity_t)
    {
        auto& cpp_e = static_cast<const doc_cpp_entity&>(*doc_entity_container::begin());
        doc.add_entity(make_heading(i, cpp_e, doc, level,
                                    p.get_output_config().is_set(output_flag::show_modules)));
    }
    else
    {
        auto heading = md_heading::make(doc, level);
        heading->add_entity(md_text::make(*heading, "Member group"));
        heading->add_entity(i.get_linker().get_anchor(*begin(), *heading));
        doc.add_entity(std::move(heading));
    }

    code_block_writer out(doc, p.get_output_config().is_set(output_flag::use_advanced_code_block));
    do_generate_synopsis(p, out, true);
    doc.add_entity(out.get_code_block());

    doc.add_entity(get_comment().get_content().clone(doc));

    if (p.get_output_config().is_set(output_flag::inline_documentation))
    {
        inline_container inlines(doc);
        for (auto& member : *this)
            inlines.add_inlines(p, i, member);
        inlines.finish(doc);
    }
}

void doc_member_group::do_generate_synopsis(const parser& p, code_block_writer& out,
                                            bool top_level) const
{
    if (!top_level && (get_comment().in_unique_member_group()
                       || p.get_output_config().is_set(output_flag::show_group_output_section))
        && get_comment().has_group_name())
    {
        out << "//=== ";
        out.write_link(top_level, get_comment().get_group_name(), begin()->get_unique_name());
        out << " ===//" << newl;
    }

    auto show_numbers = top_level && !get_comment().in_unique_member_group()
                        && p.get_output_config().is_set(output_flag::show_group_member_id);
    auto first = true;
    auto i     = 0u;
    for (auto& child : static_cast<const doc_entity_container&>(*this))
    {
        if (first)
            first = false;
        else if (top_level)
            out << blankl;
        else
            out << newl;

        ++i;
        if (show_numbers)
        {
            out << '(' << i << ')';
            out.fill_ws(p.get_output_config().get_tab_width() / 2);
            out.indent(p.get_output_config().get_tab_width() / 2 + 3);
        }

        detail::generation_access::do_generate_synopsis(child, p, out, top_level);

        if (show_numbers)
            out.unindent(p.get_output_config().get_tab_width() / 2 + 3);
    }
}

namespace
{
    bool is_blacklisted(const entity_blacklist& blacklist, const cpp_entity& e, const comment* c)
    {
        if (blacklist.is_blacklisted(entity_blacklist::synopsis, e))
            // only ignore synopsis blacklisted entities
            // documentation blacklists should still be included
            return true;
        else if (c && c->is_excluded())
            return true;

        return false;
    }

    using standardese::index;

    void handle_children(const parser& p, const index& i, std::string output_file,
                         doc_container_cpp_entity& cont);

    doc_entity_ptr handle_child_impl(const parser& p, const index& i, const doc_entity& parent,
                                     const cpp_entity& e, cpp_access_specifier_t cur_access,
                                     const std::string& output_file)
    {
        auto& blacklist       = p.get_output_config().get_blacklist();
        auto  extract_private = blacklist.is_set(entity_blacklist::extract_private);
        if (!extract_private && cur_access == cpp_private
            && detail::get_virtual(e) == cpp_virtual_none)
            // entity is private and not virtual, and private entities are excluded
            return nullptr;

        auto comment = p.get_comment_registry().lookup_comment(e, &parent);
        if (is_blacklisted(blacklist, e, comment))
            // entity is blacklisted
            return nullptr;

        if (is_inline_cpp_entity(e.get_entity_type()))
            return detail::make_doc_ptr<doc_inline_cpp_entity>(&parent, e, comment);
        else if (is_leave_entity(e))
            return detail::make_doc_ptr<doc_leave_cpp_entity>(&parent, e, comment);
        else if (is_container_entity(e))
        {
            auto container = detail::make_doc_ptr<doc_container_cpp_entity>(&parent, e, comment);
            handle_children(p, i, output_file, *container);
            return std::move(container);
        }
        else
            assert(e.get_entity_type() == cpp_entity::invalid_t
                   || e.get_entity_type() == cpp_entity::access_specifier_t);

        return nullptr;
    }

    void handle_group(const parser& p, doc_container_cpp_entity& parent, doc_entity_ptr entity)
    {
        doc_member_group* group = nullptr;
        if (!entity->get_comment().in_unique_member_group())
        {
            for (auto& child : parent)
            {
                if (child.get_entity_type() == doc_entity::member_group_t)
                {
                    auto& g = static_cast<doc_member_group&>(child);
                    if (g.group_id() == entity->get_comment().member_group_id())
                    {
                        group = &g;
                        break;
                    }
                }
            }
        }

        if (group)
        {
            if (entity->has_comment() && !entity->get_comment().empty())
                p.get_logger()->warn("Comment for entity '{}' has documentation text that is "
                                     "ignored because it is in a group.",
                                     entity->get_unique_name().c_str());
            group->add_entity(std::move(entity));
        }
        else
        {
            // need a new group
            auto group = doc_member_group::make(parent, entity->get_comment());
            group->add_entity(std::move(entity));
            parent.add_entity(std::move(group));
        }
    }

    bool handle_child(const parser& p, const index& i, doc_container_cpp_entity& parent,
                      const cpp_entity& e, cpp_access_specifier_t cur_access,
                      const std::string& output_file)
    {
        auto entity = handle_child_impl(p, i, parent, e, cur_access, output_file);
        if (!entity)
            return false;

        auto e_ptr = entity.get();
        if (entity->has_comment() && entity->get_comment().in_member_group())
            handle_group(p, parent, std::move(entity));
        else
            parent.add_entity(std::move(entity));
        i.register_entity(p, *e_ptr, output_file);
        return true;
    }

    void handle_children(const parser& p, const index& i, std::string output_file,
                         doc_container_cpp_entity& cont)
    {
        auto& entity = cont.get_cpp_entity();
        switch (entity.get_entity_type())
        {
        case cpp_entity::language_linkage_t:
            for (auto& child : static_cast<const cpp_language_linkage&>(entity))
                handle_child(p, i, cont, child, cpp_public, std::move(output_file));
            break;
        case cpp_entity::namespace_t:
            for (auto& child : static_cast<const cpp_namespace&>(entity))
                handle_child(p, i, cont, child, cpp_public, std::move(output_file));
            break;

        case cpp_entity::enum_t:
            for (auto& child : static_cast<const cpp_enum&>(entity))
                handle_child(p, i, cont, child, cpp_public, output_file);
            break;

        case cpp_entity::function_template_t:
        {
            auto any_added = false;
            for (auto& child :
                 static_cast<const cpp_function_template&>(entity).get_template_parameters())
            {
                if (handle_child(p, i, cont, child, cpp_public, output_file))
                    any_added = true;
            }
            if (!any_added)
                cont.set_cpp_entity(*get_function(entity));
        }
        // fallthrough
        case cpp_entity::function_t:
        case cpp_entity::member_function_t:
        case cpp_entity::conversion_op_t:
        case cpp_entity::constructor_t:
        case cpp_entity::function_template_specialization_t:
            for (auto& child : get_function(entity)->get_parameters())
                handle_child(p, i, cont, child, cpp_public, output_file);
            break;

        case cpp_entity::class_template_t:
        case cpp_entity::class_template_partial_specialization_t:
            if (entity.get_entity_type() == cpp_entity::class_template_t)
            {
                auto any_added = false;
                for (auto& child :
                     static_cast<const cpp_class_template&>(entity).get_template_parameters())
                {
                    if (handle_child(p, i, cont, child, cpp_public, output_file))
                        any_added = true;
                }
                if (!any_added)
                    cont.set_cpp_entity(*get_class(entity));
            }
            else
            {
                auto any_added = false;
                for (auto& child :
                     static_cast<const cpp_class_template_partial_specialization&>(entity)
                         .get_template_parameters())
                {
                    if (handle_child(p, i, cont, child, cpp_public, output_file))
                        any_added = true;
                }
                if (!any_added)
                    cont.set_cpp_entity(*get_class(entity));
            }
        // fallthrough
        case cpp_entity::class_template_full_specialization_t:
        case cpp_entity::class_t:
        {
            auto& c = *get_class(entity);
            for (auto& child : c.get_bases())
                handle_child(p, i, cont, child, child.get_access(), output_file);

            auto cur_access = c.get_class_type() == cpp_class_t ? cpp_private : cpp_public;
            for (auto& child : c)
            {
                if (child.get_entity_type() == cpp_entity::access_specifier_t)
                {
                    cur_access = static_cast<const cpp_access_specifier&>(child).get_access();
                    cont.add_entity(
                        detail::make_doc_ptr<doc_cpp_access_entity>(&cont, child, nullptr));
                }
                else
                    handle_child(p, i, cont, child, cur_access, output_file);
            }
            break;
        }

        case cpp_entity::alias_template_t:
            for (auto& child :
                 static_cast<const cpp_alias_template&>(entity).get_template_parameters())
                handle_child(p, i, cont, child, cpp_public, output_file);
            break;

        case cpp_entity::destructor_t:
            break;

        default:
            assert(false);
            break;
        }
    }
}

doc_ptr<doc_file> doc_file::parse(const parser& p, const index& i, std::string output_name,
                                  const cpp_file& f)
{
    auto file_ptr =
        detail::make_doc_ptr<doc_container_cpp_entity>(nullptr, f, p.get_comment_registry()
                                                                       .lookup_comment(f, nullptr));
    if (file_ptr->has_comment() && file_ptr->get_comment().has_unique_name_override())
        output_name = file_ptr->get_comment().get_unique_name_override();

    auto res = detail::make_doc_ptr<doc_file>(output_name, std::move(file_ptr));
    res->file_->set_parent(res.get());

    for (auto& child : f)
    {
        auto entity = handle_child_impl(p, i, *res, child, cpp_public, output_name);
        if (!entity)
            continue;
        auto e_ptr = entity.get();
        if (entity->has_comment() && entity->get_comment().in_member_group())
            handle_group(p, *res->file_, std::move(entity));
        else
            res->file_->add_entity(std::move(entity));
        i.register_entity(p, *e_ptr, output_name);
    }
    i.register_entity(p, *res->file_, std::move(output_name));

    return res;
}

void doc_file::do_generate_documentation(const parser& p, const index& i, md_document& doc,
                                         unsigned level) const
{
    file_->do_generate_documentation(p, i, doc, level);
}

void doc_file::do_generate_synopsis(const parser& p, code_block_writer& out, bool top_level) const
{
    file_->do_generate_synopsis(p, out, top_level);
}
