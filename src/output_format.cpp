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

    void write(output_stream_base& output, const cmark_str& str)
    {
        for (auto ptr = str.get(); *ptr; ++ptr)
            output.write_char(*ptr);
    }
}

void output_format_xml::do_render(output_stream_base& output, const md_entity& entity)
{
    cmark_str str(cmark_render_xml(entity.get_node(), CMARK_OPT_NOBREAKS));
    write(output, str);
}

void output_format_html::do_render(output_stream_base& output, const md_entity& entity)
{
    cmark_str str(cmark_render_html(entity.get_node(), CMARK_OPT_NOBREAKS));
    write(output, str);
}

void output_format_markdown::do_render(output_stream_base& output, const md_entity& entity)
{
    cmark_str str(cmark_render_commonmark(entity.get_node(), CMARK_OPT_NOBREAKS, width_));
    write(output, str);
}

void output_format_man::do_render(output_stream_base& output, const md_entity& entity)
{
    cmark_str str(cmark_render_man(entity.get_node(), CMARK_OPT_NOBREAKS, width_));
    write(output, str);
}

void output_format_latex::do_render(output_stream_base& output, const md_entity& entity)
{
    cmark_str str(cmark_render_latex(entity.get_node(), CMARK_OPT_NOBREAKS, width_));
    write(output, str);
}

std::unique_ptr<output_format_base> standardese::make_output_format(const std::string& name,
                                                                    unsigned           width)
{
    if (name == output_format_markdown::name())
        return std::unique_ptr<output_format_markdown>(new output_format_markdown(width));
    else if (name == output_format_html::name())
        return std::unique_ptr<output_format_html>(new output_format_html);
    else if (name == output_format_latex::name())
        return std::unique_ptr<output_format_latex>(new output_format_latex(width));
    else if (name == output_format_man::name())
        return std::unique_ptr<output_format_man>(new output_format_man(width));
    else if (name == output_format_xml::name())
        return std::unique_ptr<output_format_xml>(new output_format_xml);
    else
        throw std::invalid_argument("unknown format '" + name + "'");

    return nullptr;
}
