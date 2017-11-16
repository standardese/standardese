// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/markup/generator.hpp>

#include <cassert>
#include <cstring>
#include <cstdio>
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

using namespace standardese::markup;

namespace
{
    class stream
    {
    public:
        explicit stream(type_safe::object_ref<std::ostream> out, std::string extension)
        : out_(out), ext_(std::move(extension)), top_level_(true), closing_newl_(false)
        {
        }

        stream(stream&& other)
        : closing_(std::move(other.closing_)),
          out_(other.out_),
          ext_(other.extension()),
          top_level_(other.top_level_),
          closing_newl_(other.closing_newl_)
        {
            other.closing_.clear();
            other.top_level_.reset();
            other.closing_newl_.reset();
        }

        stream& operator=(const stream&) = delete;

        ~stream()
        {
            close();
        }

        const std::string& extension() const noexcept
        {
            return ext_;
        }

        // opens a new tag
        // destructor stream object will write closing one
        stream open_tag(bool open_newl, bool closing_newl, const char* tag)
        {
            return open_tag(open_newl, closing_newl, tag, block_id());
        }

        // opens tag with id and classes
        stream open_tag(bool open_newl, bool closing_newl, const char* tag, block_id id,
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

            return stream(out_, extension(), tag, closing_newl);
        }

        stream open_link(const char* title, const char* url)
        {
            *out_ << "<a href=\"";
            write_url(url);
            *out_ << '"';
            if (*title)
            {
                *out_ << " title=\"";
                write(title);
                *out_ << '"';
            }
            *out_ << ">";
            return stream(out_, extension(), "a", false);
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
            // implements rule 1 here: https://www.owasp.org/index.php/XSS_(Cross_Site_Scripting)_Prevention_Cheat_Sheet
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
                    *out_ << "&#x27;";
                else if (c == '/')
                    *out_ << "&#x2F;";
                else
                    *out_ << c;
            }
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
        explicit stream(type_safe::object_ref<std::ostream> out, std::string extension,
                        std::string closing, bool closing_newl)
        : closing_(std::move(closing)),
          out_(out),
          ext_(std::move(extension)),
          top_level_(false),
          closing_newl_(closing_newl)
        {
        }

        static bool needs_url_escaping(char c)
        {
            // don't escape reserved URL characters
            // don't escape safe URL characters
            char safe[] = "-_.+!*(),%#@?=;:/,+$"
                          "0123456789"
                          "abcdefghijklmnopqrstuvwxyz"
                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            return std::strchr(safe, c) == nullptr;
        }

        void write_url(const char* url)
        {
            for (auto ptr = url; *ptr; ++ptr)
            {
                auto c = *ptr;
                if (c == '&')
                    *out_ << "&amp;";
                else if (c == '\'')
                    *out_ << "&#x27";
                else if (needs_url_escaping(c))
                {
                    char buf[3];
                    std::snprintf(buf, 3, "%02X", unsigned(c));
                    *out_ << "%";
                    *out_ << buf;
                }
                else
                    *out_ << c;
            }
        }

        std::string                         closing_;
        type_safe::object_ref<std::ostream> out_;
        std::string                         ext_;
        type_safe::flag                     top_level_, closing_newl_;
    };

    void write_entity(stream& s, const entity& e);

    template <typename T>
    void write_children(stream& s, const T& container)
    {
        for (auto& child : container)
            write_entity(s, child);
    }

    void write_document(stream& s, const document_entity& doc)
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

    void write(stream& s, const main_document& doc)
    {
        write_document(s, doc);
    }

    void write(stream& s, const subdocument& doc)
    {
        write_document(s, doc);
    }

    void write(stream& s, const template_document& doc)
    {
        auto section = s.open_tag(true, true, "section", block_id(), "template-document");
        write_children(s, doc);
    }

    void write(stream& s, const code_block& cb, bool is_synopsis = false);
    void write_list_item(stream& s, const list_item_base& item);

    // write synopsis and sections
    template <class Documentation>
    void write_documentation(stream& s, const Documentation& doc)
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
            type_safe::deferred_construction<stream> dl;
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

    void write_module(stream& s, const std::string& module)
    {
        auto span = s.open_tag(false, false, "span", block_id(), "module");
        span.write("[");
        span.write(module);
        span.write("]");
    }

    void write(stream& s, const file_documentation& doc)
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

    void write_doc_header(stream& s, const documentation_entity& doc, const char* class_name)
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

    void write(stream& s, const entity_documentation& doc)
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

    void write(stream& s, const entity_index_item& item);
    void write(stream& s, const namespace_documentation& doc);

    void write_index_child(stream& s, const block_entity& child)
    {
        if (child.kind() == entity_kind::entity_index_item)
            write(s, static_cast<const entity_index_item&>(child));
        else if (child.kind() == entity_kind::namespace_documentation)
            write(s, static_cast<const namespace_documentation&>(child));
        else
            assert(false);
    }

    void write(stream& s, const namespace_documentation& doc)
    {
        auto item = s.open_tag(true, true, "li", doc.id(), "namespace-documentation");

        write_doc_header(item, doc, "namespace-documentation-heading");
        write_documentation(item, doc);

        auto list = item.open_tag(true, true, "ul", block_id(), "namespace-members");
        for (auto& child : doc)
            write_index_child(list, child);
    }

    void write(stream& s, const module_documentation& doc)
    {
        auto item = s.open_tag(true, true, "li", doc.id(), "module-documentation");

        write_doc_header(item, doc, "module-documentation-heading");
        write_documentation(item, doc);

        auto list = item.open_tag(true, true, "ul", block_id(), "module-members");
        for (auto& child : doc)
            write(list, child);
    }

    void write_term_description(stream& s, const term& t, const description* desc, block_id id,
                                const char* class_name);

    void write(stream& s, const entity_index_item& item)
    {
        auto li = s.open_tag(true, true, "li", item.id(), "entity-index-item");
        write_term_description(li, item.entity(), item.brief() ? &item.brief().value() : nullptr,
                               block_id(), "");
    }

    void write(stream& s, const heading& h, const char* tag = "h4");

    void write_index_child(stream& s, const entity_index_item& item)
    {
        write(s, item);
    }

    void write_index_child(stream& s, const module_documentation& doc)
    {
        write(s, doc);
    }

    template <class Index>
    void write_index(stream& s, const Index& index, const char* class_name)
    {
        auto ul = s.open_tag(true, true, "ul", index.id(), class_name);
        write(ul, index.heading(), "h1");
        for (auto& child : index)
            write_index_child(ul, child);
    }

    void write(stream& s, const file_index& index)
    {
        write_index(s, index, "file-index");
    }

    void write(stream& s, const entity_index& index)
    {
        write_index(s, index, "entity-index");
    }

    void write(stream& s, const module_index& index)
    {
        write_index(s, index, "module-index");
    }

    void write(stream& s, const heading& h, const char* tag)
    {
        auto heading = s.open_tag(false, true, tag, h.id());
        write_children(heading, h);
    }

    void write(stream& s, const subheading& h)
    {
        auto heading = s.open_tag(false, true, "h5", h.id());
        write_children(heading, h);
    }

    void write(stream& s, const paragraph& p)
    {
        auto paragraph = s.open_tag(false, true, "p", p.id());
        write_children(paragraph, p);
    }

    void write_term_description(stream& s, const term& t, const description* desc, block_id id,
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

    void write_list_item(stream& s, const list_item_base& item)
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
            write_term_description(s, term, &description, item.id(), "term-description-item");
        }
        else
            assert(false);
    }

    void write(stream& s, const unordered_list& list)
    {
        auto ul = s.open_tag(true, true, "ul", list.id());
        for (auto& item : list)
            write_list_item(ul, item);
    }

    void write(stream& s, const ordered_list& list)
    {
        auto ol = s.open_tag(true, true, "ol", list.id());
        for (auto& item : list)
            write_list_item(ol, item);
    }

    void write(stream& s, const block_quote& quote)
    {
        auto bq = s.open_tag(true, true, "blockquote", quote.id());
        write_children(bq, quote);
    }

    void write(stream& s, const code_block& cb, bool is_synopsis)
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

    void write(stream& s, const code_block::keyword& text)
    {
        s.write_html(R"(<span class="kwd">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(stream& s, const code_block::identifier& text)
    {
        s.write_html(R"(<span class="typ dec var fun">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(stream& s, const code_block::string_literal& text)
    {
        s.write_html(R"(<span class="str">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(stream& s, const code_block::int_literal& text)
    {
        s.write_html(R"(<span class="lit">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(stream& s, const code_block::float_literal& text)
    {
        s.write_html(R"(<span class="lit">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(stream& s, const code_block::punctuation& text)
    {
        s.write_html(R"(<span class="pun">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(stream& s, const code_block::preprocessor& text)
    {
        s.write_html(R"(<span class="pre">)");
        s.write(text.string());
        s.write_html("</span>");
    }

    void write(stream& s, const thematic_break&)
    {
        s.write_newl();
        s.write_html("<hr />\n");
    }

    void write(stream& s, const text& t)
    {
        s.write(t.string());
    }

    void write(stream& s, const emphasis& emph)
    {
        auto em = s.open_tag(false, false, "em");
        write_children(em, emph);
    }

    void write(stream& s, const strong_emphasis& emph)
    {
        auto strong = s.open_tag(false, false, "strong");
        write_children(strong, emph);
    }

    void write(stream& s, const code& c)
    {
        auto code = s.open_tag(false, false, "code");
        write_children(code, c);
    }

    void write(stream& s, const soft_break&)
    {
        s.write("\n");
    }

    void write(stream& s, const hard_break&)
    {
        s.write_html("<br/>\n");
    }

    void write(stream& s, const external_link& link)
    {
        auto a = s.open_link(link.title().c_str(), link.url().as_str().c_str());
        write_children(a, link);
    }

    void write(stream& s, const documentation_link& link)
    {
        if (link.internal_destination())
        {
            auto url = link.internal_destination()
                           .value()
                           .document()
                           .map(&output_name::file_name, s.extension().c_str())
                           .value_or("");
            url += "#standardese-" + link.internal_destination().value().id().as_str();

            auto a = s.open_link(link.title().c_str(), url.c_str());
            write_children(a, link);
        }
        else if (link.external_destination())
        {
            auto url = link.external_destination().value().as_str();

            auto a = s.open_link(link.title().c_str(), url.c_str());
            write_children(a, link);
        }
        else
            // only write link content
            write_children(s, link);
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

generator standardese::markup::html_generator(const std::string& extension) noexcept
{
    return [extension](std::ostream& out, const entity& e) {
        stream s(type_safe::ref(out), extension);
        write_entity(s, e);
    };
}
