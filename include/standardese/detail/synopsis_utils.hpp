// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_SYNOPSIS_UTILS_HPP_INCLUDED
#define STANDARDESE_DETAIL_SYNOPSIS_UTILS_HPP_INCLUDED

#include <standardese/cpp_type.hpp>
#include <standardese/doc_entity.hpp>
#include <standardese/parser.hpp>
#include <standardese/output.hpp>

namespace standardese
{
    class cpp_class;
    class cpp_function_base;
} // namespace standardese

namespace standardese
{
    namespace detail
    {
        bool is_blacklisted(const parser& p, const cpp_entity& e);

        inline bool is_blacklisted(const parser& p, const cpp_entity& e, const comment* c)
        {
            if (c && c->is_excluded())
                return true;
            else if (p.get_output_config()
                         .get_blacklist()
                         .is_blacklisted(entity_blacklist::synopsis, e))
                return true;
            return e.get_semantic_parent() ? is_blacklisted(p, *e.get_semantic_parent()) : false;
        }

        inline bool is_blacklisted(const parser& p, const doc_entity& e)
        {
            if (e.has_comment() && e.get_comment().is_excluded())
                return true;
            else if (p.get_output_config()
                         .get_blacklist()
                         .is_blacklisted(entity_blacklist::synopsis, e))
                return true;
            return e.has_parent() ? is_blacklisted(p, e.get_parent()) : false;
        }

        inline bool is_blacklisted(const parser& p, const cpp_entity& e)
        {
            auto comment = p.get_comment_registry().lookup_comment(e, nullptr);
            return is_blacklisted(p, e, comment);
        }

        template <CXCursorKind Kind>
        bool is_blacklisted(const parser& par, const basic_cpp_entity_ref<Kind>& ref)
        {
            auto target = ref.get(par.get_entity_registry());
            if (!target)
                return false;
            return is_blacklisted(par, *target);
        }

        inline bool is_blacklisted(const parser& par, const cpp_type_ref& ref)
        {
            auto decl   = ref.get_declaration();
            auto entity = par.get_entity_registry().try_lookup(decl);
            if (!entity)
                return false;
            return is_blacklisted(par, *entity);
        }

        struct ref_name
        {
            string name;
            string unique_name;

            ref_name(string name, string uname = "") : name(name), unique_name(uname)
            {
            }
        };

        template <CXCursorKind Kind>
        ref_name get_ref_name(const parser& par, const basic_cpp_entity_ref<Kind>& ref)
        {
            if (is_blacklisted(par, ref))
                return string(par.get_output_config().get_hidden_name());
            auto entity = ref.get(par.get_entity_registry());
            if (!entity)
                return ref.get_name();
            return ref_name(ref.get_name(), entity->get_unique_name());
        }

        inline string get_ref_name(const parser& par, const cpp_type_ref& ref)
        {
            if (is_blacklisted(par, ref))
                return par.get_output_config().get_hidden_name();
            return ref.get_name();
        }

        void write_type_value_default(const parser& par, code_block_writer& out, bool top_level,
                                      const cpp_type_ref& type, const cpp_name& name,
                                      const cpp_name& unique_name, const std::string& def = "",
                                      bool variadic = false);

        void write_template_parameters(const parser& par, code_block_writer& out,
                                       const doc_container_cpp_entity& cont);

        void write_class_name(code_block_writer& out, bool top_level, const cpp_name& name,
                              const cpp_name& unique_name, int class_type);
        void write_bases(const parser& par, code_block_writer& out,
                         const doc_container_cpp_entity& cont, const cpp_class& c);

        void write_prefix(code_block_writer& out, int virtual_flag, bool constexpr_f,
                          bool explicit_f = false);
        void write_parameters(const parser& par, code_block_writer& out, bool top_level,
                              const doc_container_cpp_entity& cont, const cpp_function_base& f);
        void write_cv_ref(code_block_writer& out, int cv, int ref);
        void write_noexcept(const char* complex_name, code_block_writer& out,
                            const cpp_function_base& f);
        void write_override_final(code_block_writer& out, int virtual_flag);
        void write_definition(code_block_writer& out, const cpp_function_base& f);
    }
} // namespace standardese::detail

#endif // STANDARDESE_SYNOPSIS_UTILS_HPP_INCLUDED
