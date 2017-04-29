// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/code_block.hpp>

#include "html_helper.hpp"

using namespace standardese::markup;

namespace
{
    void write_highlighted(std::string& result, const char* classes, const std::string& text)
    {
        result += R"(<span class=")";
        result += classes;
        result += R"(">)";
        result += detail::escape_html(text);
        result += "</span>";
    }
}

void code_block::keyword_tag::append_html(std::string& result, const std::string& text)
{
    write_highlighted(result, "kwd", text);
}

void code_block::identifier_tag::append_html(std::string& result, const std::string& text)
{
    write_highlighted(result, "typ dec var fun", text);
}

void code_block::string_literal_tag::append_html(std::string& result, const std::string& text)
{
    write_highlighted(result, "str", text);
}

void code_block::int_literal_tag::append_html(std::string& result, const std::string& text)
{
    write_highlighted(result, "lit", text);
}

void code_block::float_literal_tag::append_html(std::string& result, const std::string& text)
{
    write_highlighted(result, "lit", text);
}

void code_block::punctuation_tag::append_html(std::string& result, const std::string& text)
{
    write_highlighted(result, "pun", text);
}

void code_block::preprocessor_tag::append_html(std::string& result, const std::string& text)
{
    write_highlighted(result, "pre", text);
}

void code_block::do_append_html(std::string& result) const
{
    detail::append_newl(result);
    result += "<pre>";
    detail::append_html_open(result, "code", id(),
                             lang_.empty() ? "" : ("language-" + lang_).c_str());

    detail::append_container(result, *this);

    result += "</code></pre>\n";
}
