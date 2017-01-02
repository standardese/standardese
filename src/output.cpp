// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/output.hpp>

#include <stack>
#include <spdlog/logger.h>

#include <standardese/comment.hpp>
#include <standardese/generator.hpp>
#include <standardese/index.hpp>
#include <standardese/linker.hpp>
#include <standardese/md_inlines.hpp>

using namespace standardese;

namespace
{
    using standardese::index;

    const char link_prefix[] = "standardese://";

    template <typename Func>
    void for_each_entity_reference(md_container& doc, Func f)
    {
        std::stack<std::pair<md_container*, const doc_entity*>> stack;
        stack.emplace(&doc, nullptr);
        while (!stack.empty())
        {
            auto cur = stack.top();
            stack.pop();

            for (auto& child : *cur.first)
            {
                if (child.get_entity_type() == md_entity::link_t)
                {
                    auto& link = static_cast<md_link&>(child);
                    if (*link.get_destination() == '\0')
                        // empty link
                        f(cur.second, link);
                    else if (std::strncmp(link.get_destination(), link_prefix,
                                          sizeof(link_prefix) - 1)
                             == 0)
                        // standardese link
                        f(cur.second, link);
                }
                else if (child.get_entity_type() == md_entity::comment_t)
                {
                    auto& comment = static_cast<md_comment&>(child);
                    stack.emplace(&comment, comment.has_entity() ? &comment.get_entity() : nullptr);
                }
                else if (is_container(child.get_entity_type()))
                {
                    auto& container = static_cast<md_container&>(child);
                    stack.emplace(&container, cur.second);
                }
            }
        }
    }

    std::string get_entity_name(const md_link& link)
    {
        if (*link.get_destination())
        {
            assert(std::strncmp(link.get_destination(), link_prefix, sizeof(link_prefix) - 1) == 0);
            std::string result = link.get_destination() + sizeof(link_prefix) - 1;
            result.pop_back();
            return result;
        }
        else if (*link.get_title())
            return link.get_title();
        else if (link.begin()->get_entity_type() != md_entity::text_t)
            // must be a text
            return "";
        auto& text = static_cast<const md_text&>(*link.begin());
        return text.get_string();
    }
}

std::string standardese::detail::normalize_url(const char* str)
{
    std::string result;
    while (*str)
    {
        auto c = *str++;
        if (c == '/')
            result += "%2F";
        else if (c == '&')
            result += "%26";
        else
            result += c;
    }
    return result;
}

void standardese::normalize_urls(const index& idx, md_container& document,
                                 const doc_entity* default_context)
{
    for_each_entity_reference(document, [&](const doc_entity* context, md_link& link) {
        auto str = get_entity_name(link);
        if (str.empty())
            return;

        if (!context)
            context = default_context;

        auto entity = context ? idx.try_name_lookup(*context, str) : idx.try_lookup(str);
        if (entity)
            link.set_destination(
                ("standardese://" + detail::normalize_url(entity->get_unique_name().c_str()) + '/')
                    .c_str());
        else
            link.set_destination(("standardese://" + str + '/').c_str());
    });
}

raw_document::raw_document(path fname, std::string text)
: file_name(std::move(fname)), text(std::move(text))
{
    auto idx = file_name.rfind('.');
    if (idx != path::npos)
    {
        file_extension = file_name.substr(idx + 1);
        file_name.erase(idx);
    }
}

void output::render(const std::shared_ptr<spdlog::logger>& logger, const md_document& doc,
                    const char* output_extension)
{
    // normalize URLs
    auto document = md_ptr<md_document>(static_cast<md_document*>(doc.clone().release()));
    normalize_urls(*index_, *document);

    // get string
    string_output str;
    format_->render(str, *document);

    // get raw_document
    raw_document raw(document->get_output_name(), str.get_string());
    render_raw(logger, raw, output_extension);
}

void output::render_template(const std::shared_ptr<spdlog::logger>& logger,
                             const template_file& templ, const documentation& doc,
                             const char* output_extension)
{
    auto document      = process_template(*parser_, *index_, templ, format_, &doc);
    document.file_name = doc.document->get_output_name();

    render_raw(logger, document, output_extension);
}

namespace
{
    unsigned get_hex_digit(char c)
    {
        static auto lookup = "0123456789ABCDEF";

        auto ptr = std::strchr(lookup, std::toupper(c));
        return ptr ? unsigned(ptr - lookup) : unsigned(-1);
    }

    unsigned parse_hex_char(const char* ptr)
    {
        auto a = get_hex_digit(ptr[1]);
        auto b = get_hex_digit(ptr[2]);
        if (a == unsigned(-1) || b == unsigned(-1))
            return 256;
        return a * 16 + b;
    }

    std::string unescape(const char* begin, const char* end)
    {
        std::string result;
        for (auto ptr = begin; ptr != end; ++ptr)
        {
            if (*ptr == '\\')
                ; // ignore
            else if (*ptr == '$')
                result += '/';
            else if (*ptr == '%')
            {
                // on the fly hexadecimal parsing
                auto number = parse_hex_char(ptr);
                if (number >= 256)
                    result += '%';
                else
                {
                    result += char(number);
                    ptr += 2; // ignore next 2 + 1 (++ptr at the end of the loop)
                }
            }
            else
                result += *ptr;
        }
        return result;
    }
}

void output::render_raw(const std::shared_ptr<spdlog::logger>& logger, const raw_document& document,
                        const char* output_extension)
{
    if (!output_extension)
        output_extension = format_->extension();

    auto extension =
        document.file_extension.empty() ? format_->extension() : document.file_extension;
    file_output output(prefix_ + document.file_name + '.' + extension);

    auto last_match = document.text.c_str();
    // while we find standardese protocol URLs starting at last_match
    while (auto match = std::strstr(last_match, link_prefix))
    {
        // write from last_match to match
        output.write_str(last_match, match - last_match);

        // write correct URL
        auto        entity_name = match + sizeof(link_prefix) - 1;
        const char* end         = std::strchr(entity_name, '/');
        if (end == nullptr)
            end = &document.text.back() + 1;

        auto name = unescape(entity_name, end);
        auto url  = index_->get_linker().get_url(*index_, nullptr, name, output_extension);
        if (url.empty())
        {
            logger->warn("unable to resolve link to an entity named '{}'", name);
            output.write_str(match, entity_name - match);
            last_match = entity_name;
        }
        else
        {
            output.write_str(url.c_str(), url.size());
            last_match = end + 1;
        }
    }
    // write remainder of file
    output.write_str(last_match, &document.text.back() + 1 - last_match);
}
