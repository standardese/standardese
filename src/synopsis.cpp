// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/synopsis.hpp>

#include <standardese/cpp_preprocessor.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

namespace
{
    void dispatch(output_base::code_block_writer &out, const cpp_entity &e, bool top_level);

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_file &f, bool)
    {
        auto need = false;
        for (auto& e : f)
        {
            if (need)
                out << blankl;
            else
                need = true;

            dispatch(out, e, false);
        }
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
