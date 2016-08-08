// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_SYNOPSIS_UTILS_HPP_INCLUDED
#define STANDARDESE_DETAIL_SYNOPSIS_UTILS_HPP_INCLUDED

#include <standardese/cpp_type.hpp>
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
        inline bool is_blacklisted(const parser& p, const cpp_entity& e)
        {
            auto& blacklist = p.get_output_config().get_blacklist();
            if (blacklist.is_blacklisted(entity_blacklist::synopsis, e))
                return true;
            auto comment = p.get_comment_registry().lookup_comment(p.get_entity_registry(), e);
            if (comment && comment->is_excluded())
                return true;
            return false;
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
            auto decl = ref.get_declaration();
            if (decl == cpp_cursor())
                return false;
            auto entity = par.get_entity_registry().try_lookup(decl);
            if (!entity)
                return false;
            return is_blacklisted(par, *entity);
        }

        template <CXCursorKind Kind>
        string get_ref_name(const parser& par, const basic_cpp_entity_ref<Kind>& ref)
        {
            if (is_blacklisted(par, ref))
                return par.get_output_config().get_hidden_name();
            return ref.get_name();
        }

        inline string get_ref_name(const parser& par, const cpp_type_ref& ref)
        {
            if (is_blacklisted(par, ref))
                return par.get_output_config().get_hidden_name();
            return ref.get_name();
        }

        template <class Container, typename T, typename Func>
        void write_range(code_block_writer& out, const Container& cont, T sep, Func f)
        {
            auto need = false;
            for (auto& e : cont)
            {
                if (need)
                    out << sep;

                need = f(out, e);
            }

            out.remove_trailing_line();
        }

        void write_type_value_default(const parser& par, code_block_writer& out,
                                      const cpp_type_ref& type, const cpp_name& name,
                                      const std::string& def = "", bool variadic = false);

        void write_class_name(code_block_writer& out, const cpp_name& name, int class_type);

        void write_bases(const parser& par, code_block_writer& out, const cpp_class& c,
                         bool extract_private);

        void write_parameters(const parser& par, code_block_writer& out, const cpp_function_base& f,
                              const cpp_name& override_name);

        void write_noexcept(code_block_writer& out, const cpp_function_base& f);

        void write_definition(code_block_writer& out, const cpp_function_base& f);

        void write_cv_ref(code_block_writer& out, int cv, int ref);

        void write_prefix(code_block_writer& out, int virtual_flag, bool constexpr_f,
                          bool explicit_f = false);

        void write_override_final(code_block_writer& out, int virtual_flag);
    }
} // namespace standardese::detail

#endif // STANDARDESE_SYNOPSIS_UTILS_HPP_INCLUDED
