// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/generator.hpp>

#include <ostream>

#include <type_safe/reference.hpp>
#include <type_safe/flag.hpp>

#include <standardese/markup/block.hpp>
#include <standardese/markup/code_block.hpp>
#include <standardese/markup/doc_section.hpp>
#include <standardese/markup/document.hpp>
#include <standardese/markup/documentation.hpp>
#include <standardese/markup/entity.hpp>
#include <standardese/markup/entity_kind.hpp>
#include <standardese/markup/heading.hpp>
#include <standardese/markup/link.hpp>
#include <standardese/markup/list.hpp>
#include <standardese/markup/paragraph.hpp>
#include <standardese/markup/phrasing.hpp>
#include <standardese/markup/quote.hpp>
#include <standardese/markup/thematic_break.hpp>

using namespace standardese::markup;

namespace
{
    class stream
    {
    public:
        stream(type_safe::object_ref<std::ostream> out, bool include_attributes = true)
        : out_(out), newl_(false), attributes_(include_attributes)
        {
        }

        stream(stream&& other)
        : closing_(std::move(other.closing_)),
          out_(other.out_),
          newl_(other.newl_),
          attributes_(other.attributes_)
        {
            other.closing_.clear();
            other.newl_.reset();
        }

        ~stream()
        {
            close();
        }

        stream& operator=(const stream&) = delete;

        enum tag_kind
        {
            block_tag,
            line_tag,
            inline_tag,
        };

        template <typename... Attributes>
        stream open_tag(tag_kind kind, const char* tag, const Attributes&... attributes)
        {
            *out_ << "<" << tag;

            if (attributes_ == true)
            {
                int fold_expr[] = {(attributes.second.empty() ?
                                        0 :
                                        (*out_ << " " << attributes.first << "=\"",
                                         write(attributes.second), *out_ << '"', 0))...,
                                   0};
                (void)fold_expr;
            }

            *out_ << ">";
            if (kind == block_tag)
                *out_ << '\n';
            return stream(*this, tag, kind != inline_tag);
        }

        // writes XML escaped text
        void write(const char* str)
        {
            for (auto ptr = str; *ptr; ++ptr)
            {
                auto c = *ptr;
                if (c == '&')
                    *out_ << "&amp;";
                else if (c == '<')
                    *out_ << "&lt;";
                else if (c == '>')
                    *out_ << "&gt;";
                else if (c == '"')
                    *out_ << "&quot;";
                else if (c == '\'')
                    *out_ << "&apos;";
                else
                    *out_ << c;
            }
        }

        void write(const std::string& str)
        {
            write(str.c_str());
        }

        // writes unescaped xml
        void write_xml(const char* str)
        {
            *out_ << str;
        }

    private:
        explicit stream(const stream& parent, std::string closing, bool newl)
        : closing_(closing), out_(parent.out_), newl_(newl), attributes_(parent.attributes_)
        {
        }

        void close()
        {
            if (closing_.empty())
                newl_.reset();
            else
            {
                *out_ << "</" << closing_ << ">";
                if (newl_.try_reset())
                    *out_ << "\n";
                closing_.clear();
            }
        }

        std::string                         closing_;
        type_safe::object_ref<std::ostream> out_;
        type_safe::flag                     newl_, attributes_;
    };

    void write_entity(stream& s, const entity& e);

    template <typename T>
    void write_children(stream& s, const T& container)
    {
        for (auto& child : container)
            write_entity(s, child);
    }

    template <typename T>
    void write_block(stream& s, const char* tag_name, const T& block)
    {
        auto tag =
            s.open_tag(stream::block_tag, tag_name, std::make_pair("id", block.id().as_str()));
        write_children(tag, block);
    }

    template <typename T>
    void write_line_block(stream& s, const char* tag_name, const T& block)
    {
        auto tag =
            s.open_tag(stream::line_tag, tag_name, std::make_pair("id", block.id().as_str()));
        write_children(tag, block);
    }

    template <typename T>
    void write_phrasing(stream& s, const char* tag_name, const T& phrasing)
    {
        auto tag = s.open_tag(stream::inline_tag, tag_name);
        write_children(tag, phrasing);
    }

    void write_document(stream& s, const document_entity& doc, const char* tag_name)
    {
        s.write_xml(R"(<?xml version="1.0" encoding="UTF-8"?>)");
        s.write_xml("\n");
        auto tag = s.open_tag(stream::block_tag, tag_name,
                              std::make_pair("output-name", doc.output_name().name()),
                              std::make_pair("title", doc.title()));
        write_children(tag, doc);
    }

    void write(stream& s, const main_document& doc)
    {
        write_document(s, doc, "main-document");
    }

    void write(stream& s, const subdocument& doc)
    {
        write_document(s, doc, "subdocument");
    }

    void write(stream& s, const template_document& doc)
    {
        write_document(s, doc, "template-document");
    }

    void write(stream& s, const heading& h);
    void write(stream& s, const code_block& cb);

    void write_documentation(stream& s, const documentation& doc, const char* tag_name)
    {
        auto tag = s.open_tag(stream::block_tag, tag_name, std::make_pair("id", doc.id().as_str()),
                              std::make_pair("module", doc.module().value_or("")));
        write(tag, doc.heading());
        write(tag, doc.synopsis());
        for (auto& sec : doc.doc_sections())
            write_entity(tag, sec);
        write_children(tag, doc);
    }

    void write(stream& s, const file_documentation& doc)
    {
        write_documentation(s, doc, "file-documentation");
    }

    void write(stream& s, const entity_documentation& doc)
    {
        write_documentation(s, doc, "entity-documentation");
    }

    void write(stream& s, const heading& h)
    {
        write_line_block(s, "heading", h);
    }

    void write(stream& s, const subheading& h)
    {
        write_line_block(s, "subheading", h);
    }

    void write(stream& s, const paragraph& p)
    {
        write_line_block(s, "paragraph", p);
    }

    void write(stream& s, const list_item& item)
    {
        write_block(s, "list-item", item);
    }

    void write(stream& s, const term& t)
    {
        write_phrasing(s, "term", t);
    }

    void write(stream& s, const description& desc)
    {
        write_phrasing(s, "description", desc);
    }

    void write(stream& s, const term_description_item& item)
    {
        auto tag = s.open_tag(stream::line_tag, "term-description-item",
                              std::make_pair("id", item.id().as_str()));
        write(tag, item.term());
        write(tag, item.description());
    }

    void write(stream& s, const unordered_list& list)
    {
        write_block(s, "unordered-list", list);
    }

    void write(stream& s, const ordered_list& list)
    {
        write_block(s, "ordered-list", list);
    }

    void write(stream& s, const block_quote& quote)
    {
        write_block(s, "block-quote", quote);
    }

    void write(stream& s, const code_block& code)
    {
        auto tag =
            s.open_tag(stream::line_tag, "code-block", std::make_pair("id", code.id().as_str()),
                       std::make_pair("language", code.language()));
        write_children(tag, code);
    }

    template <typename T>
    void write_cb(stream& s, const char* tag_name, const T& cb)
    {
        auto tag = s.open_tag(stream::inline_tag, tag_name);
        tag.write(cb.string());
    }

    void write(stream& s, const code_block::keyword& cb)
    {
        write_cb(s, "code-block-keyword", cb);
    }

    void write(stream& s, const code_block::identifier& cb)
    {
        write_cb(s, "code-block-identifier", cb);
    }

    void write(stream& s, const code_block::string_literal& cb)
    {
        write_cb(s, "code-block-string-literal", cb);
    }

    void write(stream& s, const code_block::int_literal& cb)
    {
        write_cb(s, "code-block-int-literal", cb);
    }

    void write(stream& s, const code_block::float_literal& cb)
    {
        write_cb(s, "code-block-float-literal", cb);
    }

    void write(stream& s, const code_block::punctuation& cb)
    {
        write_cb(s, "code-block-punctuation", cb);
    }

    void write(stream& s, const code_block::preprocessor& cb)
    {
        write_cb(s, "code-block-preprocessor", cb);
    }

    void write(stream& s, const brief_section& section)
    {
        write_line_block(s, "brief-section", section);
    }

    void write(stream& s, const details_section& section)
    {
        write_block(s, "details-section", section);
    }

    void write(stream& s, const inline_section& section)
    {
        auto tag =
            s.open_tag(stream::line_tag, "inline-section", std::make_pair("name", section.name()));
        write_children(tag, section);
    }

    void write(stream& s, const list_section& section)
    {
        auto tag =
            s.open_tag(stream::block_tag, "list-section", std::make_pair("name", section.name()));
        write(tag, section.list());
    }

    void write(stream& s, const thematic_break&)
    {
        s.open_tag(stream::line_tag, "thematic-break");
    }

    void write(stream& s, const text& t)
    {
        s.write(t.string());
    }

    void write(stream& s, const emphasis& phrasing)
    {
        write_phrasing(s, "emphasis", phrasing);
    }

    void write(stream& s, const strong_emphasis& phrasing)
    {
        write_phrasing(s, "strong-emphasis", phrasing);
    }

    void write(stream& s, const code& phrasing)
    {
        write_phrasing(s, "code", phrasing);
    }

    void write(stream& s, const soft_break&)
    {
        s.open_tag(stream::line_tag, "soft-break");
    }

    void write(stream& s, const hard_break&)
    {
        s.open_tag(stream::line_tag, "hard-break");
    }

    void write(stream& s, const external_link& link)
    {
        auto tag =
            s.open_tag(stream::inline_tag, "external-link", std::make_pair("title", link.title()),
                       std::make_pair("url", link.url()));
        write_children(tag, link);
    }

    void write(stream& s, const internal_link& link)
    {
        auto tag =
            s.open_tag(stream::inline_tag, "internal-link", std::make_pair("title", link.title()),
                       std::make_pair("destination-document",
                                      link.destination()
                                          .document()
                                          .value_or(output_name::from_name(""))
                                          .name()),
                       std::make_pair("destination-id", link.destination().id().as_str()));
        write_children(tag, link);
    }

    void write_entity(stream& s, const entity& e)
    {
        switch (e.kind())
        {
#define STANDARDESE_DETAIL_HANDLE(Kind)                                                            \
    case entity_kind::Kind:                                                                        \
        write(s, static_cast<const Kind&>(e));                                                     \
        break;
#define STANDARDESE_DETAIL_HANDLE_CODE_BLOCK(Kind)                                                 \
    case entity_kind::code_block_##Kind:                                                           \
        write(s, static_cast<const code_block::Kind&>(e));                                         \
        break;
            STANDARDESE_DETAIL_HANDLE(main_document)
            STANDARDESE_DETAIL_HANDLE(subdocument)
            STANDARDESE_DETAIL_HANDLE(template_document)

            STANDARDESE_DETAIL_HANDLE(file_documentation)
            STANDARDESE_DETAIL_HANDLE(entity_documentation)

            STANDARDESE_DETAIL_HANDLE(heading)
            STANDARDESE_DETAIL_HANDLE(subheading)

            STANDARDESE_DETAIL_HANDLE(paragraph)

            STANDARDESE_DETAIL_HANDLE(list_item)

            STANDARDESE_DETAIL_HANDLE(term)
            STANDARDESE_DETAIL_HANDLE(description)
            STANDARDESE_DETAIL_HANDLE(term_description_item)

            STANDARDESE_DETAIL_HANDLE(unordered_list)
            STANDARDESE_DETAIL_HANDLE(ordered_list)

            STANDARDESE_DETAIL_HANDLE(block_quote)

            STANDARDESE_DETAIL_HANDLE(code_block)
            STANDARDESE_DETAIL_HANDLE_CODE_BLOCK(keyword)
            STANDARDESE_DETAIL_HANDLE_CODE_BLOCK(identifier)
            STANDARDESE_DETAIL_HANDLE_CODE_BLOCK(string_literal)
            STANDARDESE_DETAIL_HANDLE_CODE_BLOCK(int_literal)
            STANDARDESE_DETAIL_HANDLE_CODE_BLOCK(float_literal)
            STANDARDESE_DETAIL_HANDLE_CODE_BLOCK(punctuation)
            STANDARDESE_DETAIL_HANDLE_CODE_BLOCK(preprocessor)

            STANDARDESE_DETAIL_HANDLE(brief_section)
            STANDARDESE_DETAIL_HANDLE(details_section)
            STANDARDESE_DETAIL_HANDLE(inline_section)
            STANDARDESE_DETAIL_HANDLE(list_section)

            STANDARDESE_DETAIL_HANDLE(thematic_break)

            STANDARDESE_DETAIL_HANDLE(text)
            STANDARDESE_DETAIL_HANDLE(emphasis)
            STANDARDESE_DETAIL_HANDLE(strong_emphasis)
            STANDARDESE_DETAIL_HANDLE(code)
            STANDARDESE_DETAIL_HANDLE(soft_break)
            STANDARDESE_DETAIL_HANDLE(hard_break)

            STANDARDESE_DETAIL_HANDLE(external_link)
            STANDARDESE_DETAIL_HANDLE(internal_link)

#undef STANDARDESE_DETAIL_HANDLE
#undef STANDARDESE_DETAIL_HANDLE_CODE_BLOCK
        }
    }
}

generator standardese::markup::xml_generator(bool include_attributes) noexcept
{
    if (include_attributes)
        return [](std::ostream& out, const entity& e) {
            stream s(type_safe::ref(out));
            write_entity(s, e);
        };
    else
        return [](std::ostream& out, const entity& e) {
            stream s(type_safe::ref(out), false);
            write_entity(s, e);
        };
}
