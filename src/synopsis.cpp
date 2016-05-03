// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/synopsis.hpp>

#include <standardese/translation_unit.hpp>

using namespace standardese;

namespace
{
    void dispatch(output_base::code_block_writer &out, const cpp_entity &e, bool top_level);

    void do_write_synopsis(output_base::code_block_writer &out, const cpp_file &f, bool top_level)
    {
        for (auto& e : f)
            dispatch(out, e, false);
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
