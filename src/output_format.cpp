// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/output_format.hpp>

#include <cmark.h>
#include <cstdlib>

#include <standardese/detail/wrapper.hpp>
#include <standardese/md_entity.hpp>

using namespace standardese;

output_format_base::~output_format_base() STANDARDESE_NOEXCEPT
{
}

namespace
{
    struct cmark_deleter
    {
        void operator()(char* str) const STANDARDESE_NOEXCEPT
        {
            std::free(str);
        }
    };

    using cmark_str = detail::wrapper<char*, cmark_deleter>;
}

void output_format_markdown::do_render(output_stream_base& output, const md_entity& entity)
{
    cmark_str str(cmark_render_commonmark(entity.get_node(), CMARK_OPT_NOBREAKS, width_));

    for (auto ptr = str.get(); *ptr; ++ptr)
        output.write_char(*ptr);
}
