// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/generator.hpp>

#include <standardese/comment.hpp>
#include <standardese/cpp_class.hpp>
#include <standardese/cpp_enum.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/synopsis.hpp>

using namespace standardese;

namespace
{
    bool is_blacklisted(cpp_entity::type t)
    {
        return t == cpp_entity::inclusion_directive_t
            || t == cpp_entity::access_specifier_t
            || t == cpp_entity::base_class_t
            || t == cpp_entity::using_declaration_t
            || t == cpp_entity::using_directive_t;
    }

    void dispatch(output_base &output, unsigned level, const cpp_entity &e)
    {
        if (is_blacklisted(e.get_entity_type()))
            return;

        switch (e.get_entity_type())
        {
            case cpp_entity::namespace_t:
                for (auto& child : static_cast<const cpp_namespace &>(e))
                    dispatch(output, level, child);
                break;

            #define STANDARDESE_DETAIL_HANDLE(name, ...) \
                case cpp_entity::name##_t: \
                    if (e.get_comment().empty()) \
                        break; \
                    generate_doc_entity(output, level, e); \
                    for (auto& child : static_cast<const cpp_##name &>(e)__VA_ARGS__) \
                        dispatch(output, level + 1, child); \
                    output.write_seperator(); \
                    break;

            STANDARDESE_DETAIL_HANDLE(class)
            STANDARDESE_DETAIL_HANDLE(class_template, .get_class())
            STANDARDESE_DETAIL_HANDLE(class_template_full_specialization, .get_class())
            STANDARDESE_DETAIL_HANDLE(class_template_partial_specialization, .get_class())

            STANDARDESE_DETAIL_HANDLE(enum)

            #undef STANDARDESE_DETAIL_HANDLE

            default:
                if (!e.get_comment().empty())
                    generate_doc_entity(output, level, e);
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

        case cpp_entity::enum_t:
            return "enumeration";
        case cpp_entity::enum_value_t:
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
    }

    return "should never get here";
}

void standardese::generate_doc_entity(output_base &output, unsigned level, const cpp_entity &e)
{
    auto type = get_entity_type_spelling(e.get_entity_type());

    output_base::heading_writer(output, level) << char(std::toupper(type[0])) << &type[1] << ' '
        << output_base::style::code_span << e.get_name() << output_base::style::code_span;

    write_synopsis(output, e);

    auto comment = comment::parser(e).finish();

    auto last_type = section_type::brief;
    output_base::paragraph_writer writer(output);
    for (auto& sec : comment.get_sections())
    {
        if (last_type != sec.type)
        {
            writer.start_new();
            if (!sec.name.empty())
                output.write_section_heading(sec.name);
        }

        writer << sec.body << newl;
        last_type = sec.type;
    }
}

void standardese::generate_doc_file(output_base &output, const cpp_file &f)
{
    generate_doc_entity(output, 1, f);

    for (auto& e : f)
        dispatch(output, 2, e);
}
