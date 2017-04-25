// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/document.hpp>

#include <fstream>

#include "html_helper.hpp"

using namespace standardese::markup;

namespace
{
    void build_html(std::string& result, const document_entity& doc)
    {
        result += "<!DOCTYPE html>\n";
        result += "<html lang=\"en\">\n";
        result += "<head>\n";
        result += "<meta charset=\"utf-8\">\n";
        result += "<title>" + detail::escape_html(doc.title()) + "</title>\n";
        result += "</head>\n";
        result += "<body>\n";

        detail::append_container(result, doc);

        result += "</body>\n";
        result += "</html>\n";
    }

    void generate_html_impl(const document_entity& doc)
    {
        std::ofstream file(doc.output_name().file_name("html"));
        file << as_html(doc);
    }
}

void main_document::do_append_html(std::string& result) const
{
    build_html(result, *this);
}

void standardese::markup::generate_html(const main_document& document)
{
    generate_html_impl(document);
}

void sub_document::do_append_html(std::string& result) const
{
    build_html(result, *this);
}

void standardese::markup::generate_html(const sub_document& document)
{
    generate_html_impl(document);
}

namespace
{
    output_name get_output_name(std::string file_name)
    {
        auto pos = file_name.find('.');
        if (pos == std::string::npos)
            return output_name::from_name(std::move(file_name));
        return output_name::from_file_name(std::move(file_name));
    }
}

void template_document::do_append_html(std::string& result) const
{
    detail::append_html_open(result, "section", block_id(""), "template-document");
    detail::append_container(result, *this);
    result += "</section>\n";
}

template_document::template_document(std::string title, std::string file_name)
: document_entity(std::move(title), get_output_name(std::move(file_name)))
{
}
