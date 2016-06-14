// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/generator.hpp>

#include <standardese/comment.hpp>
#include <standardese/cpp_class.hpp>
#include <standardese/cpp_enum.hpp>
#include <standardese/cpp_function.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

namespace
{
    template <class Entity>
    cpp_access_specifier_t get_default_access(const Entity&)
    {
        return cpp_public;
    }

    cpp_access_specifier_t get_default_access(const cpp_class& e)
    {
        return e.get_class_type() == cpp_class_t ? cpp_private : cpp_public;
    }

    cpp_access_specifier_t get_default_access(const cpp_class_template& e)
    {
        return get_default_access(e.get_class());
    }

    cpp_access_specifier_t get_default_access(const cpp_class_template_full_specialization& e)
    {
        return get_default_access(e.get_class());
    }

    cpp_access_specifier_t get_default_access(const cpp_class_template_partial_specialization& e)
    {
        return get_default_access(e.get_class());
    }

    void dispatch(const parser& p, output_base& output, unsigned level, const cpp_entity& e);

    template <class Entity, class Container>
    void handle_container(const parser& p, output_base& output, unsigned level, const Entity& e,
                          const Container& container)
    {
        auto& blacklist = p.get_output_config().get_blacklist();
        if (blacklist.is_set(entity_blacklist::require_comment) && e.get_comment().empty())
            return;

        generate_doc_entity(p, output, level, doc_entity(p, e));

        auto cur_access = get_default_access(e);
        for (auto& child : container)
        {
            if (child.get_entity_type() == cpp_entity::access_specifier_t)
                cur_access =
                    static_cast<const cpp_access_specifier&>(static_cast<const cpp_entity&>(child))
                        .get_access();
            else if (blacklist.is_set(entity_blacklist::extract_private)
                     || cur_access != cpp_private || detail::is_virtual(child))
                dispatch(p, output, level + 1, child);
        }

        output.write_seperator();
    }

    void dispatch(const parser& p, output_base& output, unsigned level, const cpp_entity& e)
    {
        auto& blacklist = p.get_output_config().get_blacklist();
        if (blacklist.is_blacklisted(entity_blacklist::documentation, e))
            return;

        switch (e.get_entity_type())
        {
        case cpp_entity::namespace_t:
            for (auto& child : static_cast<const cpp_namespace&>(e))
                dispatch(p, output, level, child);
            break;

#define STANDARDESE_DETAIL_HANDLE(name, ...)                                                       \
    case cpp_entity::name##_t:                                                                     \
        handle_container(p, output, level, static_cast<const cpp_##name&>(e),                      \
                         static_cast<const cpp_##name&>(e) __VA_ARGS__);                           \
        break;

#define STANDARDESE_DETAIL_NOTHING

            STANDARDESE_DETAIL_HANDLE(class, STANDARDESE_DETAIL_NOTHING)
            STANDARDESE_DETAIL_HANDLE(class_template, .get_class())
            STANDARDESE_DETAIL_HANDLE(class_template_full_specialization, .get_class())
            STANDARDESE_DETAIL_HANDLE(class_template_partial_specialization, .get_class())

            STANDARDESE_DETAIL_HANDLE(enum, STANDARDESE_DETAIL_NOTHING)

#undef STANDARDESE_DETAIL_HANDLE
#undef STANDARDESE_DETAIL_NOTHING

        default:
            if (blacklist.is_set(entity_blacklist::require_comment) && e.get_comment().empty())
                break;
            generate_doc_entity(p, output, level, doc_entity(p, e));
            break;
        }
    }
}

const char* standardese::get_entity_type_spelling(cpp_entity::type t)
{
    switch (t)
    {
    case cpp_entity::file_t:
        return "header file";

    case cpp_entity::inclusion_directive_t:
        return "inclusion directive";
    case cpp_entity::macro_definition_t:
        return "macro";

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
        return "enumeration constant";

    case cpp_entity::variable_t:
    case cpp_entity::member_variable_t:
    case cpp_entity::bitfield_t:
        return "variable";

    case cpp_entity::function_parameter_t:
        return "parameter";
    case cpp_entity::function_t:
    case cpp_entity::member_function_t:
        return "function";
    case cpp_entity::conversion_op_t:
        return "conversion operator";
    case cpp_entity::constructor_t:
        return "constructor";
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
        return "class";
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

    return "should never get here";
}

void standardese::generate_doc_entity(const parser& p, output_base& output, unsigned level,
                                      const doc_entity& doc)
{
    auto& e       = doc.get_cpp_entity();
    auto& comment = doc.get_comment();

    auto type = get_entity_type_spelling(e.get_entity_type());

    output_base::heading_writer(output, level) << char(std::toupper(type[0])) << &type[1] << ' '
                                               << output_base::style::code_span << e.get_full_name()
                                               << output_base::style::code_span;

    write_synopsis(p, output, doc);

    auto                          last_type = section_type::brief;
    output_base::paragraph_writer writer(output);
    for (auto& sec : comment.get_sections())
    {
        if (last_type != sec.type)
        {
            writer.start_new();
            auto& section_name = p.get_output_config().get_section_name(sec.type);
            if (!section_name.empty())
                output.write_section_heading(section_name);
        }

        writer << sec.body << newl;
        last_type = sec.type;
    }
}

void standardese::generate_doc_file(const parser& p, output_base& output, const cpp_file& f)
{
    generate_doc_entity(p, output, 1, doc_entity(p, f));

    for (auto& e : f)
        dispatch(p, output, 2, e);
}
