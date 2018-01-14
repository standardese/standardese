// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/generator.hpp>

#include <cassert>
#include <ostream>

#include <type_safe/deferred_construction.hpp>
#include <type_safe/flag.hpp>
#include <type_safe/reference.hpp>

#include <standardese/markup/block.hpp>
#include <standardese/markup/code_block.hpp>
#include <standardese/markup/doc_section.hpp>
#include <standardese/markup/document.hpp>
#include <standardese/markup/documentation.hpp>
#include <standardese/markup/entity.hpp>
#include <standardese/markup/entity_kind.hpp>
#include <standardese/markup/heading.hpp>
#include <standardese/markup/index.hpp>
#include <standardese/markup/link.hpp>
#include <standardese/markup/list.hpp>
#include <standardese/markup/paragraph.hpp>
#include <standardese/markup/phrasing.hpp>
#include <standardese/markup/quote.hpp>
#include <standardese/markup/thematic_break.hpp>

#include "escape.hpp"

using namespace standardese::markup;

namespace
{
    class html_stream
    {
    public:
        explicit html_stream(type_safe::object_ref<std::ostream> out, std::string prefix,
                             std::string extension)
        : out_(out),
          prefix_(std::move(prefix)),
          ext_(std::move(extension)),
          top_level_(true),
          closing_newl_(false)
        {
        }

        html_stream(html_stream&& other)
        : closing_(std::move(other.closing_)),
          out_(other.out_),
          prefix_(std::move(other.prefix_)),
          ext_(other.extension()),
          top_level_(other.top_level_),
          closing_newl_(other.closing_newl_)
        {
            other.closing_.clear();
            other.top_level_.reset();
            other.closing_newl_.reset();
        }

        html_stream& operator=(const html_stream&) = delete;

        ~html_stream()
        {
            close();
        }

        const std::string& extension() const noexcept
        {
            return ext_;
        }

        // opens a new tag
        // destructor stream object will write closing one
        html_stream open_tag(bool open_newl, bool closing_newl, const char* tag)
        {
            return open_tag(open_newl, closing_newl, tag, block_id());
        }

        // opens tag with id and classes
        html_stream open_tag(bool open_newl, bool closing_newl, const char* tag, block_id id,
                             const char* classes = "")
        {
            *out_ << "<" << tag;
            if (!id.empty())
            {
                *out_ << " id=\"standardese-";
                write(id.as_str().c_str());
                *out_ << '"';
            }
            if (*classes)
            {
                *out_ << " class=\"standardese-";
                write(classes);
                *out_ << '"';
            }
            *out_ << ">";

            if (open_newl)
                *out_ << "\n";

            return html_stream(out_, prefix_, extension(), tag, closing_newl);
        }

        html_stream open_link(const char* title, const char* url, bool prefix)
        {
            *out_ << "<a href=\"";
            if (prefix)
                detail::write_html_url(*out_, prefix_.c_str());
            detail::write_html_url(*out_, url);
            *out_ << '"';
            if (*title)
            {
                *out_ << " title=\"";
                write(title);
                *out_ << '"';
            }
            *out_ << ">";
            return html_stream(out_, prefix_, extension(), "a", false);
        }

        // closes the current tag
        void close()
        {
            if (!closing_.empty())
                *out_ << "</" << closing_ << ">";
            closing_.clear();
            if (closing_newl_.try_reset())
                *out_ << '\n';
        }

        void write_newl()
        {
            if (!top_level_.try_reset())
                *out_ << '\n';
        }

        // writes HTML text, properly escaped
        void write(const char* str)
        {
            detail::write_html_text(*out_, str);
        }

        void write(const std::string& str)
        {
            write(str.c_str());
        }

        // writes raw HTML code
        void write_html(const char* html)
        {
            *out_ << html;
        }

    private:
        explicit html_stream(type_safe::object_ref<std::ostream> out, std::string prefix,
                             std::string extension, std::string closing, bool closing_newl)
        : closing_(std::move(closing)),
          out_(out),
          prefix_(std::move(prefix)),
          ext_(std::move(extension)),
          top_level_(false),
          closing_newl_(closing_newl)
        {
        }

        std::string                         closing_;
        type_safe::object_ref<std::ostream> out_;
        std::string                         prefix_, ext_;
        type_safe::flag                     top_level_, closing_newl_;
    };

    void write_entity(html_stream& s, const entity& e);

    template <typename T>
    void write_children(html_stream& s, const T& container)
    {
        for (auto& child : container)
            write_entity(s, child);
    }

    void write_document(html_stream& s, const document_entity& doc)
    {
        s.write_html("<!DOCTYPE html>\n");
        s.write_html("<html lang=\"en\">\n");
        s.write_html("<head>\n");
        s.write_html("<meta charset=\"utf-8\">\n");
        {
            auto title = s.open_tag(false, false, "title");
            title.write(doc.title());
        }
        s.write_html("\n</head>\n");
        s.write_html("<body>\n");

        write_children(s, doc);

        s.write_html("</body>\n");
        s.write_html("</html>\n");
    }

    void write(html_stream& s, const main_document& doc)
    {
        write_document(s, doc);
    }

    void write(html_stream& s, const subdocument& doc)
    {
        write_document(s, doc);
    }

    void write(html_stream& s, const template_document& doc)
    {
        auto section = s.open_tag(true, true, "section", block_id(), "template-document");
        write_children(s, doc);
    }

    void write(html_stream& s, const code_block& cb, bool is_synopsis = false);
    void write_list_item(html_stream& s, const list_item_base& item);

    // write synopsis and sections
    template <class Documentation>
    void write_documentation(html_stream& s, const Documentation& doc)
    {
        auto id_prefix = doc.id().empty() ? "" : doc.id().as_str() + "-";

        // write synopsis
        if (doc.synopsis())
            write(s, doc.synopsis().value(), true);

        // write brief section
        if (auto brief = doc.brief_section())
        {
            auto p = s.open_tag(false, true, "p", brief.value().id(), "brief-section");
            write_children(p, brief.value());
        }

        // write inline sections
        {
            type_safe::deferred_construction<html_stream> dl;
            for (auto& section : doc.doc_sections())
                if (section.kind() != entity_kind::inline_section)
                    continue;
                else
                {
                    auto& sec = static_cast<const inline_section&>(section);

                    if (!dl)
                        dl.emplace(s.open_tag(true, true, "dl",
                                              block_id(id_prefix + "inline-sections"),
                                              "inline-sections"));
                    // write section name
                    auto dt = dl.value().open_tag(false, true, "dt");
                    dt.write(sec.name());
                    dt.write(":");
                    dt.close();

                    // write section content
                    auto dd = dl.value().open_tag(false, true, "dd");
                    write_children(dd, sec);
                }
        }

        // write details section
        if (auto details = doc.details_section())
            write_children(s, details.value());

        // write list sections
        for (auto& section : doc.doc_sections())
            if (section.kind() != entity_kind::list_section)
                continue;
            else
            {
                auto& list = static_cast<const list_section&>(section);

                // heading
                auto h4 = s.open_tag(false, true, "h4", block_id(), "list-section-heading");
                h4.write(list.name());
                h4.close();

                // list
                auto ul = s.open_tag(true, true, "ul", list.id(), "list-section");
                for (auto& item : list)
                    write_list_item(ul, item);
            }
    }

    void write_module(html_stream& s, const std::string& module)
    {
        auto span = s.open_tag(false, false, "span", block_id(), "module");
        span.write("[");
        span.write(module);
        span.write("]");
    }

    void write(html_stream& s, const file_documentation& doc)
    {
        // <article> represents the actual content of a website
        auto article = s.open_tag(true, true, "article", doc.id(), "file-documentation");

        if (doc.header())
        {
            auto& header = doc.header().value();

            auto heading = article.open_tag(false, true, "h1", header.heading().id(),
                                            "file-documentation-heading");
            write_children(heading, header.heading());

            if (header.module())
                write_module(heading, header.module().value());
        }

        write_documentation(article, doc);

        write_children(article, doc);
    }

    const char* get_documentation_heading_tag(const documentation_entity& doc)
    {
        for (auto cur = doc.parent(); cur; cur = cur.value().parent())
            if (cur.value().kind() == entity_kind::entity_documentation
                || cur.value().kind() == entity_kind::namespace_documentation)
                // use h3 when entity has a parent entity
                return "h3";
        // return h2 otherwise
        return "h2";
    }

    void write_doc_header(html_stream& s, const documentation_entity& doc, const char* class_name)
    {
        if (doc.header())
        {
            auto& header = doc.header().value();

            auto heading = s.open_tag(false, true, get_documentation_heading_tag(doc),
                                      header.heading().id(), class_name);
            write_children(heading, header.heading());

            if (header.module())
                write_module(heading, header.module().value());
        }
    }

    void write(html_stream& s, const entity_documentation& doc)
    {
        // <section> represents a semantic section in the website
        auto section = s.open_tag(true, true, "section", doc.id(), "entity-documentation");

        write_doc_header(section, doc, "entity-documentation-heading");
        write_documentation(section, doc);
        write_children(section, doc);

        section.close();

        if (doc.header())
            s.write_html(R"(<hr class="standardese-entity-documentation-break" />)"
                         "\n");
    }

    void write(html_stream& s, const entity_index_item& item);
    void write(html_stream& s, const namespace_documentation& doc);

    void write_index_child(html_stream& s, const block_entity& child)
    {
        if (child.kind() == entity_kind::entity_index_item)
            write(s, static_cast<const entity_index_item&>(child));
        else if (child.kind() == entity_kind::namespace_documentation)
            write(s, static_cast<const namespace_documentation&>(child));
        else
            assert(false);
    }

    void write(html_stream& s, const namespace_documentation& doc)
    {
        auto item = s.open_tag(true, true, "li", doc.id(), "namespace-documentation");

        write_doc_header(item, doc, "namespace-documentation-heading");
        write_documentation(item, doc);

        auto list = item.open_tag(true, true, "ul", block_id(), "namespace-members");
        for (auto& child : doc)
            write_index_child(list, child);
    }

    void write(html_stream& s, const module_documentation& doc)
    {
        auto item = s.open_tag(true, true, "li", doc.id(), "module-documentation");

        write_doc_header(item, doc, "module-documentation-heading");
        write_documentation(item, doc);

        auto list = item.open_tag(true, true, "ul", block_id(), "module-members");
        for (auto& child : doc)
            write(list, child);
    }

    void write_term_description(html_stream& s, const term& t, const description* desc, block_id id,
                                const char* class_name);

    void write(html_stream& s, const entity_index_item& item)
    {
        auto li = s.open_tag(true, true, "li", item.id(), "entity-index-item");
        write_term_description(li, item.entity(), item.brief() ? &item.brief().value() : nullptr,
                               block_id(), "");
    }

    void write(html_stream& s, const heading& h, const char* tag = "h4");

    void write_index_child(html_stream& s, const entity_index_item& item)
    {
        write(s, item);
    }

    void write_index_child(html_stream& s, const module_documentation& doc)
    {
        write(s, doc);
    }

    template <class Index>
    void write_index(html_stream& s, const Index& index, const char* class_name)
    {
        auto ul = s.open_tag(true, true, "ul", index.id(), class_name);
        write(ul, index.heading(), "h1");
        for (auto& child : index)
            write_index_child(ul, child);
    }

    void write(html_stream& s, const file_index& index)
    {
        write_index(s, index, "file-index");
    }

    void write(html_stream& s, const entity_index& index)
    {
        write_index(s, index, "entity-index");
    }

    void write(html_stream& s, const module_index& index)
    {
        write_index(s, index, "module-index");
    }

    void write(html_stream& s, const heading& h, const char* tag)
    {
        auto heading = s.open_tag(false, true, tag, h.id());
        write_children(heading, h);
    }

    void write(html_stream& s, const subheading& h)
    {
        auto heading = s.open_tag(false, true, "h5", h.id());
        write_children(heading, h);
    }

    void write(html_stream& s, const paragraph& p)
    {
        auto paragraph = s.open_tag(false, true, "p", p.id());
        write_children(paragraph, p);
    }

    void write_term_description(html_stream& s, const term& t, const description* desc, block_id id,
                                const char* class_name)
    {
        auto dl = s.open_tag(true, true, "dl", std::move(id), class_name);

        auto dt = s.open_tag(false, true, "dt");
        write_children(dt, t);
        dt.close();

        if (desc)
        {
            auto dd = s.open_tag(false, true, "dd");
            dd.write_html("&mdash; ");
            write_children(dd, *desc);
        }
    }

    void write_list_item(html_stream& s, const list_item_base& item)
    {
        auto li = s.open_tag(true, true, "li", item.id());

        if (item.kind() == entity_kind::list_item)
        {
            write_children(li, static_cast<const list_item&>(item));
        }
        else if (item.kind() == entity_kind::term_description_item)
        {
            auto& term        = static_cast<const term_description_item&>(item).term();
            auto& description = static_cast<const term_description_item&>(item).description();
            write_term_description(li, term, &description, item.id(), "term-description-item");
        }
        else
            assert(false);
    }

    void write(html_stream& s, const unordered_list& list)
    {
        auto ul = s.open_tag(true, true, "ul", list.id());
        for (auto& item : list)
            write_list_item(ul, item);
    }

    void write(html_stream& s, const ordered_list& list)
    {
        auto ol = s.open_tag(true, true, "ol", list.id());
        for (auto& item : list)
            write_list_item(ol, item);
    }

    void write(html_stream& s, const block_quote& quote)
    {
        auto bq = s.open_tag(true, true, "blockquote", quote.id());
        write_children(bq, quote);
    }

    void write(html_stream& s, const code_block& cb, bool is_synopsis)
    {
        std::string classes;
        if (!cb.language().empty())
            classes += "language-" + cb.language();
        if (is_synopsis)
            classes += " standardese-entity-synopsis";

        auto pre  = s.open_tag(false, true, "pre", block_id());
        auto code = pre.open_tag(false, false, "code", cb.id(), classes.c_str());
        write_children(code, cb);
    }

    void write(html_stream& s, const code_block::keyword& text)
    {
        s.write_html(R"(<span class="kwd">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(html_stream& s, const code_block::identifier& text)
    {
        s.write_html(R"(<span class="typ dec var fun">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(html_stream& s, const code_block::string_literal& text)
    {
        s.write_html(R"(<span class="str">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(html_stream& s, const code_block::int_literal& text)
    {
        s.write_html(R"(<span class="lit">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(html_stream& s, const code_block::float_literal& text)
    {
        s.write_html(R"(<span class="lit">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(html_stream& s, const code_block::punctuation& text)
    {
        s.write_html(R"(<span class="pun">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(html_stream& s, const code_block::preprocessor& text)
    {
        s.write_html(R"(<span class="pre">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(html_stream& s, const thematic_break&)
    {
        s.write_newl();
        s.write_html("<hr />\n");
    }

    void write(html_stream& s, const text& t)
    {
        s.write(t.string());
    }

    void write(html_stream& s, const emphasis& emph)
    {
        auto em = s.open_tag(false, false, "em");
        write_children(em, emph);
    }

    void write(html_stream& s, const strong_emphasis& emph)
    {
        auto strong = s.open_tag(false, false, "strong");
        write_children(strong, emph);
    }

    void write(html_stream& s, const code& c)
    {
        auto code = s.open_tag(false, false, "code");
        write_children(code, c);
    }

    void write(html_stream& s, const verbatim& v)
    {
        s.write_html(v.content().c_str());
    }

    void write(html_stream& s, const soft_break&)
    {
        s.write("\n");
    }

    void write(html_stream& s, const hard_break&)
    {
        s.write_html("<br/>\n");
    }

    void write(html_stream& s, const external_link& link)
    {
        auto a = s.open_link(link.title().c_str(), link.url().as_str().c_str(), false);
        write_children(a, link);
    }

    void write(html_stream& s, const documentation_link& link)
    {
        if (link.internal_destination())
        {
            auto url = link.internal_destination()
                           .value()
                           .document()
                           .map(&output_name::file_name, s.extension().c_str())
                           .value_or("");
            url += "#standardese-" + link.internal_destination().value().id().as_str();

            auto a = s.open_link(link.title().c_str(), url.c_str(), true);
            write_children(a, link);
        }
        else if (link.external_destination())
        {
            auto url = link.external_destination().value().as_str();

            auto a = s.open_link(link.title().c_str(), url.c_str(), false);
            write_children(a, link);
        }
        else
            // only write link content
            write_children(s, link);
    }

    void write_entity(html_stream& s, const entity& e)
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

            STANDARDESE_DETAIL_HANDLE(file_index)
            STANDARDESE_DETAIL_HANDLE(entity_index)
            STANDARDESE_DETAIL_HANDLE(module_index)

            STANDARDESE_DETAIL_HANDLE(heading)
            STANDARDESE_DETAIL_HANDLE(subheading)

            STANDARDESE_DETAIL_HANDLE(paragraph)

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

            STANDARDESE_DETAIL_HANDLE(thematic_break)

            STANDARDESE_DETAIL_HANDLE(text)
            STANDARDESE_DETAIL_HANDLE(emphasis)
            STANDARDESE_DETAIL_HANDLE(strong_emphasis)
            STANDARDESE_DETAIL_HANDLE(code)
            STANDARDESE_DETAIL_HANDLE(verbatim)
            STANDARDESE_DETAIL_HANDLE(soft_break)
            STANDARDESE_DETAIL_HANDLE(hard_break)

            STANDARDESE_DETAIL_HANDLE(external_link)
            STANDARDESE_DETAIL_HANDLE(documentation_link)

#undef STANDARDESE_DETAIL_HANDLE
#undef STANDARDESE_DETAIL_HANDLE_CODE_BLOCK

        case entity_kind::namespace_documentation:
        case entity_kind::module_documentation:
        case entity_kind::entity_index_item:
        case entity_kind::list_item:
        case entity_kind::term:
        case entity_kind::description:
        case entity_kind::term_description_item:
        case entity_kind::brief_section:
        case entity_kind::details_section:
        case entity_kind::inline_section:
        case entity_kind::list_section:
            assert(!static_cast<bool>("can't use this entity stand-alone"));
            break;
        }
    }
}

generator standardese::markup::html_generator(const std::string& prefix,
                                              const std::string& extension) noexcept
{
    return [prefix, extension](std::ostream& out, const entity& e) {
        html_stream s(type_safe::ref(out), prefix, extension);
        write_entity(s, e);
    };
}
