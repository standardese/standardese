// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
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

void detail::synopsis_access::do_generate_synopsis(const doc_entity& e, const parser& p,
                                                   code_block_writer& out, bool top_level)
{
    e.do_generate_synopsis(p, out, top_level);
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

    md_ptr<md_anchor> get_anchor(const doc_cpp_entity& e, const md_entity& parent)
    {
        auto id = detail::escape_unique_name(e.get_unique_name().c_str());
        return md_anchor::make(parent, id.c_str());
    }

    md_ptr<md_heading> make_heading(const doc_cpp_entity& e, const md_entity& parent,
                                    unsigned level)
    {
        auto heading = md_heading::make(parent, level);

        auto type     = get_entity_type_spelling(e.get_cpp_entity());
        auto text_str = fmt::format("{}{} ", char(std::toupper(type[0])), &type[1]);
        auto text     = md_text::make(*heading, text_str.c_str());
        heading->add_entity(std::move(text));

        auto code = md_code::make(*heading, e.get_cpp_entity().get_full_name().c_str());
        heading->add_entity(std::move(code));

        heading->add_entity(get_anchor(e, *heading));

        return heading;
    }
}

void doc_cpp_entity::do_generate_documentation_base(const parser& p, md_document& doc,
                                                    unsigned level) const
{
    doc.add_entity(make_heading(*this, doc, level));

    code_block_writer out(doc);
    do_generate_synopsis(p, out, true);
    doc.add_entity(out.get_code_block());

    if (has_comment())
        doc.add_entity(get_comment().get_content().clone(doc));
}

cpp_name doc_cpp_entity::do_get_unique_name() const
{
    return entity_->get_unique_name();
}

cpp_name doc_cpp_entity::do_get_index_name() const
{
    if (get_cpp_entity_type() == cpp_entity::namespace_t)
        return entity_->get_full_name();
    auto name = std::string(entity_->get_name().c_str());
    if (auto func = get_function(*entity_))
        name += func->get_signature().c_str();
    return name;
}

cpp_name doc_entity::do_get_file_name() const
{
    auto cur = this;
    while (cur->parent_)
        cur = cur->parent_;
    assert(cur->get_cpp_entity_type() == cpp_entity::file_t);
    return cur->do_get_file_name();
}

cpp_name doc_entity::get_unique_name() const
{
    return has_comment() && get_comment().has_unique_name_override() ?
               get_comment().get_unique_name_override() :
               do_get_unique_name();
}

void doc_inline_cpp_entity::do_generate_documentation(const parser& p, md_document& doc,
                                                      unsigned level) const
{
    assert(!p.get_output_config().inline_documentation());
    do_generate_documentation_base(p, doc, level);
}

namespace
{
    md_ptr<md_list_item> get_inline_item(const parser& p, const doc_inline_cpp_entity& doc_e,
                                         const md_entity& parent)
    {
        assert(doc_e.has_comment());
        auto item = md_list_item::make(parent);

        // generate "heading"
        auto item_paragraph = md_paragraph::make(*item);
        item_paragraph->add_entity(get_anchor(doc_e, *item_paragraph));
        item_paragraph->add_entity(md_code::make(*item, doc_e.get_cpp_entity().get_name().c_str()));
        item_paragraph->add_entity(md_text::make(*item, " - "));

        // generate content
        auto first = true;
        for (auto& container : doc_e.get_comment().get_content())
        {
            if (container.get_entity_type() != md_entity::paragraph_t)
            {
                p.get_logger()->warn("inline comment for entity '{}' has markup that is ignored.",
                                     doc_e.get_cpp_entity().get_full_name().c_str());
                continue;
            }
            else if (first)
                first = false;
            else
                item_paragraph->add_entity(md_soft_break::make(*item_paragraph));

            for (auto& child : static_cast<const md_container&>(container))
                item_paragraph->add_entity(child.clone(*item));
        }
        item->add_entity(std::move(item_paragraph));

        return item;
    }
}

void doc_inline_cpp_entity::do_generate_documentation_inline(const parser& p, md_list& list) const
{
    assert(p.get_output_config().inline_documentation());
    if (has_comment())
    {
        // inline documentation with comment
        auto item = get_inline_item(p, *this, list);
        list.add_entity(std::move(item));
    }
}

namespace
{
    template <class EnumValue>
    void do_write_synopsis_enum_value(code_block_writer& out, const EnumValue& e)
    {
        out << e.get_name();
        if (e.is_explicitly_given())
            out << " = " << e.get_value();
    }

    void do_write_synopsis_member_variable(const parser& p, code_block_writer& out,
                                           const cpp_member_variable_base& v)
    {
        if (v.is_mutable())
            out << "mutable ";

        detail::write_type_value_default(p, out, v.get_type(), v.get_name());

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
        out << "typename";
        if (param.is_variadic())
            out << " ...";
        if (!param.get_name().empty())
            out << ' ' << param.get_name();
        if (param.has_default_type())
            out << " = " << detail::get_ref_name(p, param.get_default_type());
    }

    void do_write_synopsis_template_param(const parser& p, code_block_writer& out,
                                          const cpp_non_type_template_parameter& param)
    {
        detail::write_type_value_default(p, out, param.get_type(), param.get_name(),
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

        out << "> typename";
        if (param.is_variadic())
            out << " ...";
        if (!param.get_name().empty())
            out << ' ' << param.get_name();
        if (param.has_default_template())
            out << " = " << detail::get_ref_name(p, param.get_default_template());
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
                                                 bool) const
{
    switch (get_cpp_entity_type())
    {
    case cpp_entity::signed_enum_value_t:
        do_write_synopsis_enum_value(out,
                                     static_cast<const cpp_signed_enum_value&>(get_cpp_entity()));
        break;
    case cpp_entity::unsigned_enum_value_t:
        do_write_synopsis_enum_value(out,
                                     static_cast<const cpp_unsigned_enum_value&>(get_cpp_entity()));
        break;
    case cpp_entity::expression_enum_value_t:
        do_write_synopsis_enum_value(out, static_cast<const cpp_expression_enum_value&>(
                                              get_cpp_entity()));
        break;

    case cpp_entity::function_parameter_t:
    {
        auto& param = static_cast<const cpp_function_parameter&>(get_cpp_entity());
        detail::write_type_value_default(p, out, param.get_type(), param.get_name(),
                                         param.get_default_value());
        break;
    }

    case cpp_entity::member_variable_t:
    case cpp_entity::bitfield_t:
        do_write_synopsis_member_variable(p, out, static_cast<const cpp_member_variable_base&>(
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
        auto& base = static_cast<const cpp_base_class&>(get_cpp_entity());
        out << to_string(base.get_access()) << ' ' << detail::get_ref_name(p, base.get_type());
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

namespace
{
    bool is_inline_entity(cpp_entity::type t)
    {
        return t == cpp_entity::base_class_t || is_parameter(t) || is_member_variable(t)
               || is_enum_value(t);
    }

    bool is_inline_entity(const cpp_entity& e)
    {
        return is_inline_entity(e.get_entity_type());
    }
}

doc_inline_cpp_entity::doc_inline_cpp_entity(const doc_entity* parent, const cpp_entity& e,
                                             const comment* c)
: doc_cpp_entity(parent, e, c)
{
    assert(is_inline_entity(e));
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

void doc_leave_cpp_entity::do_generate_documentation(const parser& p, md_document& doc,
                                                     unsigned level) const
{
    do_generate_documentation_base(p, doc, level);
}

namespace
{
    void do_write_synopsis(const parser& par, code_block_writer& out, const cpp_namespace_alias& ns)
    {
        out << "namespace " << ns.get_name() << " = " << detail::get_ref_name(par, ns.get_target())
            << ';';
    }

    void do_write_synopsis(const parser& par, code_block_writer& out, const cpp_using_directive& u)
    {
        out << "using namespace " << detail::get_ref_name(par, u.get_target()) << ';';
    }

    void do_write_synopsis(const parser& par, code_block_writer& out,
                           const cpp_using_declaration& u)
    {
        out << "using " << detail::get_ref_name(par, u.get_target()) << ';';
    }

    void do_write_synopsis(const parser& par, code_block_writer& out, const cpp_type_alias& a)
    {
        out << "using " << a.get_name() << " = " << detail::get_ref_name(par, a.get_target())
            << ';';
    }

    void do_write_synopsis(const parser& par, code_block_writer& out, const cpp_variable& v)
    {
        if (v.get_ast_parent().get_entity_type() == cpp_entity::class_t)
            out << "static ";

        if (v.is_thread_local())
            out << "thread_local ";

        detail::write_type_value_default(par, out, v.get_type(), v.get_name(), v.get_initializer());
        out << ';';
    }

    void do_write_synopsis(const parser&, code_block_writer& out, const cpp_inclusion_directive& i)
    {
        out << "#include ";
        out << (i.get_kind() == cpp_inclusion_directive::system ? '<' : '"');
        out << i.get_file_name();
        out << (i.get_kind() == cpp_inclusion_directive::system ? '>' : '"');
    }

    void do_write_synopsis(const parser&, code_block_writer& out, const cpp_macro_definition& m)
    {
        out << "#define " << m.get_name() << m.get_parameter_string() << ' ' << m.get_replacement();
    }
}

void doc_leave_cpp_entity::do_generate_synopsis(const parser& p, code_block_writer& out, bool) const
{
    switch (get_cpp_entity_type())
    {
    case cpp_entity::macro_definition_t:
        do_write_synopsis(p, out, static_cast<const cpp_macro_definition&>(get_cpp_entity()));
        break;
    case cpp_entity::inclusion_directive_t:
        do_write_synopsis(p, out, static_cast<const cpp_inclusion_directive&>(get_cpp_entity()));
        break;

    case cpp_entity::namespace_alias_t:
        do_write_synopsis(p, out, static_cast<const cpp_namespace_alias&>(get_cpp_entity()));
        break;
    case cpp_entity::using_directive_t:
        do_write_synopsis(p, out, static_cast<const cpp_using_directive&>(get_cpp_entity()));
        break;
    case cpp_entity::using_declaration_t:
        do_write_synopsis(p, out, static_cast<const cpp_using_declaration&>(get_cpp_entity()));
        break;

    case cpp_entity::type_alias_t:
        do_write_synopsis(p, out, static_cast<const cpp_type_alias&>(get_cpp_entity()));
        break;

    case cpp_entity::variable_t:
        do_write_synopsis(p, out, static_cast<const cpp_variable&>(get_cpp_entity()));
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
    void add_list(md_document& doc, md_ptr<md_list> list, const char* name)
    {
        if (list->empty())
            return;

        auto paragraph = md_paragraph::make(doc);
        paragraph->add_entity(md_strong::make(*paragraph, name));
        doc.add_entity(std::move(paragraph));
        doc.add_entity(std::move(list));
    }
}

void doc_container_cpp_entity::do_generate_documentation(const parser& p, md_document& doc,
                                                         unsigned level) const
{
    // generate heading + synopsis + comment
    do_generate_documentation_base(p, doc, level);

    // add inline entity comments
    if (p.get_output_config().inline_documentation())
    {
        auto parameters  = md_list::make_bullet(doc);
        auto bases       = md_list::make_bullet(doc);
        auto members     = md_list::make_bullet(doc);
        auto enum_values = md_list::make_bullet(doc);

        for (auto& child : *this)
        {
            if (is_parameter(child.get_cpp_entity_type()))
                child.do_generate_documentation_inline(p, *parameters);
            else if (child.get_cpp_entity_type() == cpp_entity::base_class_t)
                child.do_generate_documentation_inline(p, *bases);
            else if (is_member_variable(child.get_cpp_entity_type()))
                child.do_generate_documentation_inline(p, *members);
            else if (is_enum_value(child.get_cpp_entity_type()))
                child.do_generate_documentation_inline(p, *enum_values);
            else
                assert(!is_inline_entity(child.get_cpp_entity_type()));
        }

        add_list(doc, std::move(parameters), "Parameters:");
        add_list(doc, std::move(bases), "Bases:");
        add_list(doc, std::move(members), "Members:");
        add_list(doc, std::move(enum_values), "Enum values:");
    }

    // add documentation for other children
    auto any_child = false;
    for (auto& child : *this)
    {
        if (p.get_output_config().inline_documentation()
            && is_inline_entity(child.get_cpp_entity_type()))
            continue;
        child.do_generate_documentation(p, doc, level + 1);
        any_child = true;
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
            detail::write_class_name(out, e.get_cpp_entity().get_name(), c.get_class_type());
        else
            detail::write_class_name(out, c.get_name(), c.get_class_type());

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

                    detail::synopsis_access::do_generate_synopsis(child, p, out, false);
                }
            }
            out.remove_trailing_line();
            out.unindent(p.get_output_config().get_tab_width());
            out << newl << '}';
        }
        out << ';';
    }

    void do_write_synopsis(const parser& p, code_block_writer& out,
                           const doc_container_cpp_entity& e, const cpp_function_base& f)
    {
        detail::write_prefix(out, detail::get_virtual(f), f.is_constexpr());

        if (f.get_entity_type() == cpp_entity::function_t)
            out << static_cast<const cpp_function&>(f).get_return_type().get_name() << ' ';
        else if (f.get_entity_type() == cpp_entity::member_function_t)
            out << static_cast<const cpp_member_function&>(f).get_return_type().get_name() << ' ';

        detail::write_parameters(p, out, e, f);

        detail::write_cv_ref(out, detail::get_cv(f), detail::get_ref_qualifier(f));
        detail::write_noexcept(out, f);

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

            detail::synopsis_access::do_generate_synopsis(child, p, out, false);
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
        out << entity.get_cpp_entity().get_name();
        if (entity.get_cpp_entity().get_name().empty() || top_level)
        {
            if (!e.get_underlying_type().get_name().empty())
                out << newl << ": " << detail::get_ref_name(p, e.get_underlying_type());

            out << newl;
            do_write_synopsis_container(p, out, entity, ",\n");
        }
        out << ';';
    }

    void do_write_synopsis(const parser& p, code_block_writer& out,
                           const doc_container_cpp_entity& entity, const cpp_namespace& ns)
    {
        if (ns.is_inline())
            out << "inline ";
        out << "namespace " << ns.get_name() << newl;
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
    // write template parameters, if there are any
    if (is_template(get_cpp_entity_type()))
        detail::write_template_parameters(p, out, *this);

    if (auto c = get_class(get_cpp_entity()))
        do_write_synopsis(p, out, top_level, *this, *c);
    else if (auto func = get_function(get_cpp_entity()))
        do_write_synopsis(p, out, *this, *func);
    else if (get_cpp_entity_type() == cpp_entity::enum_t)
        do_write_synopsis(p, out, top_level, *this, static_cast<const cpp_enum&>(get_cpp_entity()));
    else if (get_cpp_entity_type() == cpp_entity::alias_template_t)
        do_write_synopsis(p, out,
                          static_cast<const cpp_alias_template&>(get_cpp_entity()).get_type());
    else if (get_cpp_entity_type() == cpp_entity::namespace_t)
        do_write_synopsis(p, out, *this, static_cast<const cpp_namespace&>(get_cpp_entity()));
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

            detail::synopsis_access::do_generate_synopsis(child, p, out, false);
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

namespace
{
    bool has_comment(const comment* c)
    {
        return c && !c->empty();
    }

    bool requires_comment(const cpp_entity& e)
    {
        return e.get_entity_type() != cpp_entity::file_t
               && e.get_entity_type() != cpp_entity::namespace_t
               && e.get_entity_type() != cpp_entity::language_linkage_t && !is_inline_entity(e);
    }

    bool is_blacklisted(const entity_blacklist& blacklist, const cpp_entity& e, const comment* c)
    {
        if (requires_comment(e) && blacklist.is_set(entity_blacklist::require_comment)
            && !has_comment(c))
            return true;
        else if (blacklist.is_blacklisted(entity_blacklist::documentation, e))
            return true;
        else if (c && c->is_excluded())
            return true;

        return false;
    }

    using standardese::index;

    void handle_children(const parser& p, const index& i, doc_container_cpp_entity& cont);

    doc_entity_ptr handle_child_impl(const parser& p, const index& i, const doc_entity& parent,
                                     const cpp_entity& e, cpp_access_specifier_t cur_access)
    {
        auto& blacklist       = p.get_output_config().get_blacklist();
        auto  extract_private = blacklist.is_set(entity_blacklist::extract_private);
        if (!extract_private && cur_access == cpp_private
            && detail::get_virtual(e) == cpp_virtual_none)
            // entity is private and not virtual, and private entities are excluded
            return nullptr;

        auto comment = p.get_comment_registry().lookup_comment(p.get_entity_registry(), e);
        if (is_blacklisted(blacklist, e, comment))
            // entity is blacklisted
            return nullptr;

        if (is_inline_entity(e))
            return detail::make_doc_ptr<doc_inline_cpp_entity>(&parent, e, comment);
        else if (is_leave_entity(e))
            return detail::make_doc_ptr<doc_leave_cpp_entity>(&parent, e, comment);
        else if (is_container_entity(e))
        {
            auto container = detail::make_doc_ptr<doc_container_cpp_entity>(&parent, e, comment);
            handle_children(p, i, *container);
            return container;
        }
        else
            assert(e.get_entity_type() == cpp_entity::invalid_t
                   || e.get_entity_type() == cpp_entity::access_specifier_t);

        return nullptr;
    }

    void handle_child(const parser& p, const index& i, doc_container_cpp_entity& parent,
                      const cpp_entity& e, cpp_access_specifier_t cur_access)
    {
        auto entity = handle_child_impl(p, i, parent, e, cur_access);
        if (!entity)
            return;
        i.register_entity(*entity);
        parent.add_entity(std::move(entity));
    }

    void handle_children(const parser& p, const index& i, doc_container_cpp_entity& cont)
    {
        auto& entity = cont.get_cpp_entity();
        switch (entity.get_entity_type())
        {
        case cpp_entity::language_linkage_t:
            for (auto& child : static_cast<const cpp_language_linkage&>(entity))
                handle_child(p, i, cont, child, cpp_public);
            break;
        case cpp_entity::namespace_t:
            for (auto& child : static_cast<const cpp_namespace&>(entity))
                handle_child(p, i, cont, child, cpp_public);
            break;

        case cpp_entity::enum_t:
            for (auto& child : static_cast<const cpp_enum&>(entity))
                handle_child(p, i, cont, child, cpp_public);
            break;

        case cpp_entity::function_template_t:
            for (auto& child :
                 static_cast<const cpp_function_template&>(entity).get_template_parameters())
                handle_child(p, i, cont, child, cpp_public);
        // fallthrough
        case cpp_entity::function_t:
        case cpp_entity::member_function_t:
        case cpp_entity::conversion_op_t:
        case cpp_entity::constructor_t:
        case cpp_entity::function_template_specialization_t:
            for (auto& child : get_function(entity)->get_parameters())
                handle_child(p, i, cont, child, cpp_public);
            break;

        case cpp_entity::class_template_t:
        case cpp_entity::class_template_partial_specialization_t:
            if (entity.get_entity_type() == cpp_entity::class_template_t)
            {
                for (auto& child :
                     static_cast<const cpp_class_template&>(entity).get_template_parameters())
                    handle_child(p, i, cont, child, cpp_public);
            }
            else
            {
                for (auto& child :
                     static_cast<const cpp_class_template_partial_specialization&>(entity)
                         .get_template_parameters())
                    handle_child(p, i, cont, child, cpp_public);
            }
        // fallthrough
        case cpp_entity::class_template_full_specialization_t:
        case cpp_entity::class_t:
        {
            auto& c = *get_class(entity);
            for (auto& child : c.get_bases())
                handle_child(p, i, cont, child, child.get_access());

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
                    handle_child(p, i, cont, child, cur_access);
            }
            break;
        }

        case cpp_entity::alias_template_t:
            for (auto& child :
                 static_cast<const cpp_alias_template&>(entity).get_template_parameters())
                handle_child(p, i, cont, child, cpp_public);
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
    auto res = detail::make_doc_ptr<doc_file>(std::move(output_name),
                                              detail::make_doc_ptr<doc_container_cpp_entity>(
                                                  nullptr, f,
                                                  p.get_comment_registry()
                                                      .lookup_comment(p.get_entity_registry(), f)));
    res->file_->set_parent(res.get());

    for (auto& child : f)
    {
        auto entity = handle_child_impl(p, i, *res, child, cpp_public);
        if (!entity)
            continue;
        i.register_entity(*entity);
        res->file_->add_entity(std::move(entity));
    }

    return res;
}

void doc_file::generate_documentation(const parser& p, md_document& doc) const
{
    do_generate_documentation(p, doc, 1u);
}

void doc_file::do_generate_documentation(const parser& p, md_document& doc, unsigned level) const
{
    file_->do_generate_documentation(p, doc, level);
}

void doc_file::do_generate_synopsis(const parser& p, code_block_writer& out, bool top_level) const
{
    file_->do_generate_synopsis(p, out, top_level);
}
