// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/md_custom.hpp>

#include <cassert>
#include <cmark.h>

#include <standardese/md_inlines.hpp>
#include <standardese/md_blocks.hpp>

using namespace standardese;

md_ptr<md_section> md_section::make(const md_entity& parent, const std::string& section_text)
{
    return detail::make_md_ptr<md_section>(parent, section_text);
}

const char* md_section::get_section_text() const STANDARDESE_NOEXCEPT
{
    assert(begin()->get_entity_type() == md_entity::emphasis_t);
    auto& emph = static_cast<const md_emphasis&>(*begin());
    assert(emph.begin()->get_entity_type() == md_entity::text_t);
    return static_cast<const md_text&>(*emph.begin()).get_string();
}

void md_section::set_section_text(const std::string& text)
{
    assert(begin()->get_entity_type() == md_entity::emphasis_t);
    auto& emph = static_cast<md_emphasis&>(*begin());
    assert(emph.begin()->get_entity_type() == md_entity::text_t);
    static_cast<md_text&>(*emph.begin()).set_string(text.c_str());

    assert(std::next(begin())->get_entity_type() == md_entity::text_t);
    static_cast<md_text&>(*std::next(begin())).set_string(text.empty() ? "" : ": ");
}

md_entity_ptr md_section::do_clone(const md_entity* parent) const
{
    assert(parent);
    return md_section::make(*parent, get_section_text());
}

md_section::md_section(const md_entity& parent, const std::string& section_text)
: md_container(get_entity_type(), cmark_node_new(CMARK_NODE_CUSTOM_INLINE), parent)
{
    auto emphasis = md_emphasis::make(*this, section_text.c_str());
    add_entity(std::move(emphasis));

    auto buffer_text = md_text::make(*this, section_text.empty() ? "" : ": ");
    add_entity(std::move(buffer_text));
}

md_ptr<md_code_block_advanced> md_code_block_advanced::make(const md_code_block& cb)
{
    return detail::make_md_ptr<md_code_block_advanced>(cb.get_parent(), cb.get_string(),
                                                       cb.get_fence_info());
}

md_ptr<md_code_block_advanced> md_code_block_advanced::make(const md_entity& parent,
                                                            const char* code, const char* lang)
{
    return detail::make_md_ptr<md_code_block_advanced>(parent, code, lang);
}

md_entity_ptr md_code_block_advanced::do_clone(const md_entity* parent) const
{
    assert(parent);
    auto node = cmark_node_new(CMARK_NODE_HTML_BLOCK);
    cmark_node_set_literal(node, cmark_node_get_literal(get_node()));
    return detail::make_md_ptr<md_code_block_advanced>(*parent, node);
}

namespace
{
    std::string get_html(const char* lang, const char* code)
    {
        std::string result;
        result += "<pre>";
        if (lang && *lang)
        {
            result += "<code class=\"language-";
            result += lang;
            result += "\">";
        }
        else
            result += "<code>";

        result += code;

        result += "</code>";
        result += "</pre>";
        return result;
    }
}

md_code_block_advanced::md_code_block_advanced(const md_entity& parent, const char* code,
                                               const char* lang)
: md_leave(get_entity_type(), cmark_node_new(CMARK_NODE_HTML_BLOCK), parent)
{
    cmark_node_set_literal(get_node(), get_html(lang, code).c_str());
}

md_code_block_advanced::md_code_block_advanced(const md_entity& parent, cmark_node* node)
: md_leave(get_entity_type(), node, parent)
{
}

md_ptr<md_inline_documentation> md_inline_documentation::make(const md_entity&   parent,
                                                              const std::string& heading)
{
    auto res = detail::make_md_ptr<md_inline_documentation>(parent);

    // heading
    auto paragraph = md_paragraph::make(*res);
    paragraph->add_entity(md_strong::make(*paragraph, (heading + ":").c_str()));
    res->md_container::add_entity(std::move(paragraph));

    // list
    res->md_container::add_entity(md_list::make_bullet(*res));

    return res;
}

bool md_inline_documentation::add_item(const char* name, const char* id,
                                       const md_container& container)
{
    assert(this->back().get_entity_type() == md_entity::list_t);
    auto& list = static_cast<md_list&>(this->back());
    auto  item = md_list_item::make(list);

    // generate "heading"
    auto item_paragraph = md_paragraph::make(*item);
    item_paragraph->add_entity(md_anchor::make(*item_paragraph, id));
    if (std::strcmp(name, "_") != 0)
    {
        std::string name_str(name);
        if (name_str.front() == '[' && name_str.back() == ']')
        {
            name_str.pop_back();

            auto link = md_link::make(*item_paragraph, "", "");
            link->add_entity(md_text::make(*link, name_str.c_str() + 1));
            item_paragraph->add_entity(std::move(link));
        }
        else
            item_paragraph->add_entity(md_code::make(*item_paragraph, name));

        if (!container.empty())
            item_paragraph->add_entity(md_text::make(*item_paragraph, " - "));
    }

    // generate content
    auto first      = true;
    auto ignore_any = false;
    for (auto& child : container)
    {
        if (is_inline(child.get_entity_type()))
            item_paragraph->add_entity(child.clone(*item_paragraph));
        else if (child.get_entity_type() != md_entity::paragraph_t)
        {
            ignore_any = true;
            continue;
        }
        else
        {
            if (first)
                first = false;
            else
                item_paragraph->add_entity(md_soft_break::make(*item_paragraph));

            for (auto& child_child : static_cast<const md_container&>(child))
                item_paragraph->add_entity(child_child.clone(*item));
        }
    }

    item->add_entity(std::move(item_paragraph));
    list.add_entity(std::move(item));

    return !ignore_any;
}

bool md_inline_documentation::empty() const STANDARDESE_NOEXCEPT
{
    assert(this->back().get_entity_type() == md_entity::list_t);
    return static_cast<const md_list&>(this->back()).empty();
}

md_entity_ptr md_inline_documentation::do_clone(const md_entity* parent) const
{
    assert(parent);

    if (empty())
        return nullptr;

    auto res = detail::make_md_ptr<md_inline_documentation>(*parent);
    for (auto& child : *this)
        res->md_container::add_entity(child.clone(*res));
    return std::move(res);
}

md_inline_documentation::md_inline_documentation(const md_entity& parent)
: md_container(get_entity_type(), cmark_node_new(CMARK_NODE_CUSTOM_BLOCK), parent)
{
}

md_list_item& md_inline_documentation::get_item() STANDARDESE_NOEXCEPT
{
    assert(back().get_entity_type() == md_entity::list_t);
    auto& list = static_cast<md_list&>(back());
    return static_cast<md_list_item&>(list.back());
}

md_entity& md_inline_documentation::add_entity(md_entity_ptr entity)
{
    auto& item = get_item();
    assert(is_container(item.back().get_entity_type()));
    static_cast<md_container&>(item.back()).add_entity(std::move(entity));

    return *this;
}

md_ptr<md_document> md_document::make(std::string name)
{
    return detail::make_md_ptr<md_document>(cmark_node_new(CMARK_NODE_DOCUMENT), std::move(name));
}

md_entity_ptr md_document::do_clone(const md_entity* parent) const
{
    assert(!parent);
    (void)parent;

    auto result = make(name_);
    for (auto& child : *this)
        result->add_entity(child.clone(*result));
    return std::move(result);
}
