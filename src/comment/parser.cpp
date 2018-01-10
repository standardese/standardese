// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment/parser.hpp>

#include <cassert>
#include <cstring>
#include <type_traits>

#include <cmark.h>
#include <cmark_extension_api.h>

#if !defined(CMARK_NODE_TYPE_PRESENT)
#error "requires GFM cmark"
#endif

#include <standardese/markup/code_block.hpp>
#include <standardese/markup/link.hpp>
#include <standardese/markup/list.hpp>
#include <standardese/markup/heading.hpp>
#include <standardese/markup/paragraph.hpp>
#include <standardese/markup/quote.hpp>
#include <standardese/markup/thematic_break.hpp>
#include <standardese/markup/entity_kind.hpp>

#include "cmark_ext.hpp"

using namespace standardese;
using namespace standardese::comment;

parser::parser(comment::config c)
: config_(std::move(c)), parser_(cmark_parser_new(CMARK_OPT_SMART))
{
    auto command_ext = detail::create_command_extension(config_);
    cmark_parser_attach_syntax_extension(parser_, command_ext);

    auto html_ext = detail::create_no_html_extension();
    cmark_parser_attach_syntax_extension(parser_, html_ext);
}

parser::~parser() noexcept
{
    auto cur = cmark_parser_get_syntax_extensions(parser_);
    while (cur)
    {
        cmark_syntax_extension_free(cmark_get_default_mem_allocator(),
                                    static_cast<cmark_syntax_extension*>(cur->data));
        cur = cur->next;
    }
    cmark_parser_free(parser_);
}

namespace
{
    class ast_root
    {
    public:
        explicit ast_root(cmark_node* root) : root_(root) {}

        ast_root(ast_root&& other) noexcept : root_(other.root_)
        {
            other.root_ = nullptr;
        }

        ~ast_root() noexcept
        {
            if (root_)
                cmark_node_free(root_);
        }

        ast_root& operator=(ast_root&& other) noexcept
        {
            ast_root tmp(std::move(other));
            std::swap(root_, tmp.root_);
            return *this;
        }

        /// \returns The node.
        cmark_node* get() const noexcept
        {
            return root_;
        }

    private:
        cmark_node* root_;
    };

    ast_root read_ast(const parser& p, const std::string& comment)
    {
        cmark_parser_feed(p.get(), comment.c_str(), comment.size());
        auto root = cmark_parser_finish(p.get());
        return ast_root(root);
    }

    template <class Builder>
    void add_children(const config& c, Builder& b, bool has_matching_entity, cmark_node* parent);

    [[noreturn]] void error(cmark_node* node, std::string msg)
    {
        throw parse_error(unsigned(cmark_node_get_start_line(node)),
                          unsigned(cmark_node_get_start_column(node)), std::move(msg));
    }

    std::unique_ptr<markup::block_quote> parse_block_quote(const config& c,
                                                           bool          has_matching_entity,
                                                           cmark_node*   node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_BLOCK_QUOTE);

        markup::block_quote::builder builder(markup::block_id{});
        add_children(c, builder, has_matching_entity, node);
        return builder.finish();
    }

    std::unique_ptr<markup::block_entity> parse_list(const config& c, bool has_matching_entity,
                                                     cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_LIST);

        auto type = cmark_node_get_list_type(node);
        if (type == CMARK_BULLET_LIST)
        {
            markup::unordered_list::builder builder(markup::block_id{});
            add_children(c, builder, has_matching_entity, node);
            return builder.finish();
        }
        else if (type == CMARK_ORDERED_LIST)
        {
            markup::ordered_list::builder builder(markup::block_id{});
            add_children(c, builder, has_matching_entity, node);
            return builder.finish();
        }
        else
            assert(false);

        return nullptr;
    }

    std::unique_ptr<markup::list_item> parse_item(const config& c, bool has_matching_entity,
                                                  cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_ITEM);

        markup::list_item::builder builder;
        add_children(c, builder, has_matching_entity, node);
        return builder.finish();
    }

    std::unique_ptr<markup::code_block> parse_code_block(const config&, bool, cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_CODE_BLOCK);
        return markup::code_block::build(markup::block_id{}, cmark_node_get_fence_info(node),
                                         cmark_node_get_literal(node));
    }

    std::unique_ptr<markup::paragraph> parse_paragraph(const config& c, bool has_matching_entity,
                                                       cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_PARAGRAPH);

        markup::paragraph::builder builder;
        add_children(c, builder, has_matching_entity, node);
        return builder.finish();
    }

    std::unique_ptr<markup::block_entity> parse_heading(const config& c, bool has_matching_entity,
                                                        cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_HEADING);

        auto level = cmark_node_get_heading_level(node);
        if (level == 1)
        {
            markup::heading::builder builder(markup::block_id{});
            add_children(c, builder, has_matching_entity, node);
            return builder.finish();
        }
        else
        {
            markup::subheading::builder builder(markup::block_id{});
            add_children(c, builder, has_matching_entity, node);
            return builder.finish();
        }
    }

    std::unique_ptr<markup::thematic_break> parse_thematic_break(const config&, bool,
                                                                 cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_THEMATIC_BREAK);
        (void)node;
        return markup::thematic_break::build();
    }

    std::unique_ptr<markup::text> parse_text(const config&, bool, cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_TEXT);
        return markup::text::build(cmark_node_get_literal(node));
    }

    std::unique_ptr<markup::soft_break> parse_softbreak(const config&, bool, cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_SOFTBREAK);
        return markup::soft_break::build();
    }

    std::unique_ptr<markup::hard_break> parse_linebreak(const config&, bool, cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_LINEBREAK);
        return markup::hard_break::build();
    }

    std::unique_ptr<markup::code> parse_code(const config&, bool, cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_CODE);
        return markup::code::build(cmark_node_get_literal(node));
    }

    std::unique_ptr<markup::emphasis> parse_emph(const config& c, bool has_matching_entity,
                                                 cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_EMPH);
        markup::emphasis::builder builder;
        add_children(c, builder, has_matching_entity, node);
        return builder.finish();
    }

    std::unique_ptr<markup::strong_emphasis> parse_strong(const config& c, bool has_matching_entity,
                                                          cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_STRONG);
        markup::strong_emphasis::builder builder;
        add_children(c, builder, has_matching_entity, node);
        return builder.finish();
    }

    std::unique_ptr<markup::external_link> parse_external_link(const config& c,
                                                               bool          has_matching_entity,
                                                               cmark_node* node, const char* title,
                                                               const char* url)
    {
        markup::external_link::builder builder(title, markup::url(url));
        add_children(c, builder, has_matching_entity, node);
        return builder.finish();
    }

    std::unique_ptr<markup::link_base> parse_link(const config& c, bool has_matching_entity,
                                                  cmark_node* node)
    {
        assert(cmark_node_get_type(node) == CMARK_NODE_LINK);

        auto title = cmark_node_get_title(node);
        auto url   = cmark_node_get_url(node);
        if (*url == '\0' && *title == '\0')
        {
            // both empty, unique name is child iff
            // child is a single text node
            auto child = cmark_node_first_child(node);
            if (child == nullptr || child != cmark_node_last_child(node)
                || cmark_node_get_type(child) != CMARK_NODE_TEXT)
                // not an entity link
                return parse_external_link(c, has_matching_entity, node, title, url);
            else
            {
                auto unique_name = cmark_node_get_literal(child);
                auto is_relative = *unique_name == '?' || *unique_name == '*';
                return markup::documentation_link::builder(unique_name)
                    .add_child(markup::code::build(is_relative ? unique_name + 1 : unique_name))
                    .finish();
            }
        }
        else if (*url == '\0')
        {
            // url is empty, unique name is title
            markup::documentation_link::builder builder(title);
            add_children(c, builder, has_matching_entity, node);
            return builder.finish();
        }
        else if (std::strncmp(url, "standardese://", std::strlen("standardese://")) == 0)
        {
            // standardese:// URL
            auto unique_name = std::string(url + std::strlen("standardese://"));
            if (!unique_name.empty() && unique_name.back() == '/')
                unique_name.pop_back();

            markup::documentation_link::builder builder(title, unique_name);
            add_children(c, builder, has_matching_entity, node);
            return builder.finish();
        }
        else
        {
            // regular link
            return parse_external_link(c, has_matching_entity, node, title, url);
        }

        assert(false);
        return nullptr;
    }

    std::unique_ptr<markup::phrasing_entity> parse_key(const char* key)
    {
        if (*key == '[')
        {
            // look for a closing ']', might have a link
            auto begin = ++key;
            while (*key && *key != ']')
                ++key;

            if (*key)
            {
                // found one
                auto unique_name = std::string(begin, std::size_t(key - begin));
                markup::documentation_link::builder link(unique_name);
                link.add_child(markup::text::build(std::move(unique_name)));
                return link.finish();
            }
        }

        // normal text
        return markup::text::build(key);
    }

    std::unique_ptr<markup::list_section> parse_list_section(const config& c,
                                                             bool          has_matching_entity,
                                                             cmark_node*&  node)
    {
        auto type = detail::get_section_type(node);

        markup::unordered_list::builder builder{markup::block_id{}};
        do
        {
            auto key = detail::get_section_key(node);
            if (key)
            {
                // key-value item
                auto term = markup::term::build(parse_key(key));

                markup::description::builder description;
                add_children(c, description, has_matching_entity,
                             cmark_node_first_child(node)); // the children of the paragraph

                builder.add_item(markup::term_description_item::build(markup::block_id(),
                                                                      std::move(term),
                                                                      description.finish()));
            }
            else
            {
                // no key-value item, just add the normal paragraph
                auto paragraph = cmark_node_first_child(node);
                builder.add_item(
                    markup::list_item::build(parse_paragraph(c, has_matching_entity, paragraph)));
            }

            node = cmark_node_next(node);
        } while (node && cmark_node_get_type(node) == detail::node_section()
                 && detail::get_section_type(node) == type);
        // went one too far
        node = cmark_node_previous(node);

        return markup::list_section::build(type, c.list_section_name(type), builder.finish());
    }

    std::unique_ptr<markup::brief_section> parse_brief(const config& c, bool has_matching_entity,
                                                       cmark_node* node)
    {
        assert(cmark_node_get_type(cmark_node_first_child(node)) == CMARK_NODE_PARAGRAPH);

        markup::brief_section::builder builder;
        add_children(c, builder, has_matching_entity, cmark_node_first_child(node));
        return builder.finish();
    }

    std::unique_ptr<markup::details_section> parse_details(const config& c,
                                                           bool          has_matching_entity,
                                                           cmark_node*   node)
    {
        markup::details_section::builder builder;
        add_children(c, builder, has_matching_entity, node);
        return builder.finish();
    }

    std::unique_ptr<markup::doc_section> parse_section(const config& c, bool has_matching_entity,
                                                       cmark_node*& node)
    {
        assert(cmark_node_get_type(node) == detail::node_section());
        switch (detail::get_section_type(node))
        {
        case section_type::brief:
            return parse_brief(c, has_matching_entity, node);

        case section_type::details:
            return parse_details(c, has_matching_entity, node);

        case section_type::requires:
        case section_type::effects:
        case section_type::synchronization:
        case section_type::postconditions:
        case section_type::returns:
        case section_type::throws:
        case section_type::complexity:
        case section_type::remarks:
        case section_type::error_conditions:
        case section_type::notes:
        case section_type::preconditions:
        case section_type::constraints:
        case section_type::diagnostics:
        case section_type::see:
        {
            if (detail::get_section_key(node))
                return parse_list_section(c, has_matching_entity, node);
            else
            {
                auto result = markup::inline_section::builder(detail::get_section_type(node),
                                                              c.inline_section_name(
                                                                  detail::get_section_type(node)));

                auto first = true;
                for (auto child = cmark_node_first_child(node); child;
                     child      = cmark_node_next(child))
                {
                    if (first)
                        first = false;
                    else
                        result.add_child(markup::soft_break::build());

                    auto paragraph = parse_paragraph(c, has_matching_entity, child);
                    for (auto& paragraph_child : *paragraph)
                        result.add_child(markup::clone(paragraph_child));
                }

                return result.finish();
            }
        }

        case section_type::count:
            assert(false);
            break;
        }

        return nullptr;
    }

    struct comment_builder
    {
        matching_entity entity;

        metadata data;

        std::unique_ptr<markup::brief_section>            brief;
        std::vector<std::unique_ptr<markup::doc_section>> sections;
        std::vector<unmatched_doc_comment>                inlines;
    };
    bool starts_with(const char* str, const char* target)
    {
        return std::strncmp(str, target, std::strlen(target)) == 0;
    }

    const char* get_single_arg(cmark_node* node, const char* cmd)
    {
        auto args = detail::get_command_arguments(node);
        for (auto ptr = args; *ptr; ++ptr)
            if (*ptr == ' ' || *ptr == '\t')
                error(node, std::string("multiple arguments given for command '") + cmd
                                + "', but only one expected");
        return args;
    }

    void skip_ws(const char*& args)
    {
        while (*args && (*args == ' ' || *args == '\t'))
            ++args;
    }

    type_safe::optional<std::string> get_next_arg(const char*& args)
    {
        std::string result;
        while (*args && *args != ' ' && *args != '\t')
            result += *args++;
        skip_ws(args);
        return result.empty() ? type_safe::nullopt : type_safe::make_optional(std::move(result));
    }

    std::string get_next_required_arg(const char*& args, cmark_node* node, const char* command)
    {
        auto maybe_arg = get_next_arg(args);
        if (!maybe_arg)
            error(node, std::string("missing required argument command '") + command + "'");
        return maybe_arg.value();
    }

    exclude_mode parse_exclude_mode(cmark_node* node)
    {
        auto args = detail::get_command_arguments(node);
        if (starts_with(args, "return"))
            return exclude_mode::return_type;
        else if (starts_with(args, "target"))
            return exclude_mode::target;
        else if (*args)
            error(node, "invalid argument for exclude");
        else
            return exclude_mode::entity;
    }

    member_group parse_group(cmark_node* node)
    {
        auto args = detail::get_command_arguments(node);

        auto name    = get_next_required_arg(args, node, "group");
        auto heading = *args ? type_safe::make_optional(std::string(args)) : type_safe::nullopt;

        if (name.front() == '-')
        {
            // name starts with -, erase it, and don't consider it a section
            name.erase(name.begin());
            return member_group(std::move(name), std::move(heading), false);
        }
        else
            return member_group(std::move(name), std::move(heading), true);
    }

    // builder is nullptr when parsing an inline comment
    void parse_command_impl(comment_builder* builder, bool has_matching_entity, metadata& data,
                            cmark_node* node)
    {
        assert(cmark_node_get_type(node) == detail::node_command());
        auto command = detail::get_command_type(node);
        switch (command)
        {
        case command_type::exclude:
            if (!data.set_exclude(parse_exclude_mode(node)))
                error(node, "multiple exclude commands for entity");
            break;
        case command_type::unique_name:
            if (!data.set_unique_name(get_single_arg(node, "unique name")))
                error(node, "multiple unique name commands for entity");
            break;
        case command_type::output_name:
            if (!data.set_output_name(get_single_arg(node, "output name")))
                error(node, "multiple output name commands for entity");
            break;
        case command_type::synopsis:
            if (!data.set_synopsis(detail::get_command_arguments(node)))
                error(node, "multiple synopsis commands for entity");
            break;

        case command_type::group:
            if (data.output_section())
                error(node, "cannot have group and output section");
            else if (!data.set_group(parse_group(node)))
                error(node, "multiple group commands for entity");
            break;
        case command_type::module:
        {
            auto module = get_single_arg(node, "module");

            if (!has_matching_entity && builder && !builder->entity.has_value())
                // non-inline comment and not remote, treat as module comment
                builder->entity = comment::module(module);
            // otherwise treat as module specification
            else if (!data.set_module(module))
                error(node, "multiple module commands for entity");
            break;
        }
        case command_type::output_section:
            if (data.group())
                error(node, "cannot have group and output section");
            else if (!data.set_output_section(detail::get_command_arguments(node)))
                error(node, "multiple output section commands for entity");
            break;

        case command_type::entity:
            if (!builder || has_matching_entity)
                error(node, "entity command not allowed for this node");
            else if (builder->entity.has_value())
                error(node, "multiple file/entity/module commands for entity");
            else
                builder->entity = remote_entity(detail::get_command_arguments(node));
            break;
        case command_type::file:
            if (!builder || has_matching_entity)
                error(node, "file command not allowed for this node");
            else if (builder->entity.has_value())
                error(node, "multiple file/entity/module commands for entity");
            else if (*detail::get_command_arguments(node))
                error(node, "arguments given to file command but none required");
            else
                builder->entity = current_file{};
            break;

        case command_type::count:
        case command_type::invalid:
            error(node, std::string("unkown command ") + detail::get_command_arguments(node));

        case command_type::end:
            error(node, detail::get_command_arguments(node));
        }
    }

    void parse_command(metadata& data, bool has_matching_entity, cmark_node* node)
    {
        parse_command_impl(nullptr, has_matching_entity, data, node);
    }

    void parse_command(comment_builder& builder, bool has_matching_entity, cmark_node* node)
    {
        parse_command_impl(&builder, has_matching_entity, builder.data, node);
    }

    template <typename T>
    void parse_command(const T&, bool, cmark_node*)
    {
        assert(!static_cast<bool>("unexpected command"));
    }

    matching_entity parse_matching(cmark_node* node)
    {
        if (!*detail::get_inline_entity(node))
            error(node, "missing entity name for inline comment");

        switch (detail::get_inline_type(node))
        {
        case inline_type::param:
        case inline_type::tparam:
            return inline_param(detail::get_inline_entity(node));
        case inline_type::base:
            return inline_base(detail::get_inline_entity(node));

        case inline_type::count:
        case inline_type::invalid:
            break;
        }

        assert(!static_cast<bool>("invalid inline type"));
        return current_file{};
    }

    void parse_inline(const config& c, comment_builder& builder, bool, cmark_node* node)
    {
        assert(cmark_node_get_type(node) == detail::node_inline());

        metadata                                          data;
        std::unique_ptr<markup::brief_section>            brief;
        std::vector<std::unique_ptr<markup::doc_section>> sections;
        for (auto child = cmark_node_first_child(node); child; child = cmark_node_next(child))
        {
            if (cmark_node_get_type(child) == detail::node_command())
                parse_command(data, true, child);
            else if (cmark_node_get_type(child) == detail::node_section())
            {
                auto section = parse_section(c, true, child);
                if (detail::get_section_type(child) == section_type::brief)
                {
                    assert(!brief);
                    brief = std::unique_ptr<markup::brief_section>(
                        static_cast<markup::brief_section*>(section.release()));
                }
                else
                    sections.push_back(std::move(section));
            }
            else
                assert(!static_cast<bool>("unexpected child"));
        }

        builder.inlines.push_back(
            unmatched_doc_comment(parse_matching(node),
                                  doc_comment(std::move(data), std::move(brief),
                                              std::move(sections))));
    }

    template <typename T>
    void parse_inline(const config&, const T&, bool, cmark_node*)
    {
        assert(!static_cast<bool>("unexpected inline"));
    }

    template <class Builder, typename T>
    auto add_child(int, Builder& b, std::unique_ptr<T> entity)
        -> decltype(void(b.add_child(std::move(entity))))
    {
        b.add_child(std::move(entity));
    }

    template <class Builder, typename T>
    void add_child(int, Builder& b, std::unique_ptr<T> entity,
                   decltype(std::declval<Builder>().add_item(std::unique_ptr<T>{}), 0) = 0)
    {
        b.add_item(std::move(entity));
    }

    void add_child(int, comment_builder& builder, std::unique_ptr<markup::doc_section> ptr)
    {
        if (ptr->kind() == markup::entity_kind::brief_section)
        {
            auto brief = std::unique_ptr<markup::brief_section>(
                static_cast<markup::brief_section*>(ptr.release()));
            if (builder.brief)
                error(nullptr, "multiple brief sections for comment");
            else
                builder.brief = std::move(brief);
        }
        else
            builder.sections.push_back(std::move(ptr));
    }

    template <class Builder, typename T>
    void add_child(short, Builder&, std::unique_ptr<T>)
    {
        assert(!static_cast<bool>("unexpected child"));
    }

    template <class Builder>
    void add_children(const config& c, Builder& b, bool has_matching_entity, cmark_node* parent)
    {
        for (auto cur = cmark_node_first_child(parent); cur; cur = cmark_node_next(cur))
        {
            if (cmark_node_get_type(cur) == detail::node_section())
                add_child(0, b, parse_section(c, has_matching_entity, cur));
            else if (cmark_node_get_type(cur) == detail::node_command())
                parse_command(b, has_matching_entity, cur);
            else if (cmark_node_get_type(cur) == detail::node_inline())
                parse_inline(c, b, has_matching_entity, cur);
            else
                switch (cmark_node_get_type(cur))
                {
                case CMARK_NODE_BLOCK_QUOTE:
                    add_child(0, b, parse_block_quote(c, has_matching_entity, cur));
                    break;
                case CMARK_NODE_LIST:
                    add_child(0, b, parse_list(c, has_matching_entity, cur));
                    break;
                case CMARK_NODE_ITEM:
                    add_child(0, b, parse_item(c, has_matching_entity, cur));
                    break;
                case CMARK_NODE_CODE_BLOCK:
                    add_child(0, b, parse_code_block(c, has_matching_entity, cur));
                    break;
                case CMARK_NODE_PARAGRAPH:
                    add_child(0, b, parse_paragraph(c, has_matching_entity, cur));
                    break;
                case CMARK_NODE_HEADING:
                    add_child(0, b, parse_heading(c, has_matching_entity, cur));
                    break;
                case CMARK_NODE_THEMATIC_BREAK:
                    add_child(0, b, parse_thematic_break(c, has_matching_entity, cur));
                    break;

                case CMARK_NODE_TEXT:
                    add_child(0, b, parse_text(c, has_matching_entity, cur));
                    break;
                case CMARK_NODE_SOFTBREAK:
                    add_child(0, b, parse_softbreak(c, has_matching_entity, cur));
                    break;
                case CMARK_NODE_LINEBREAK:
                    add_child(0, b, parse_linebreak(c, has_matching_entity, cur));
                    break;
                case CMARK_NODE_CODE:
                    add_child(0, b, parse_code(c, has_matching_entity, cur));
                    break;
                case CMARK_NODE_EMPH:
                    add_child(0, b, parse_emph(c, has_matching_entity, cur));
                    break;
                case CMARK_NODE_STRONG:
                    add_child(0, b, parse_strong(c, has_matching_entity, cur));
                    break;
                case CMARK_NODE_LINK:
                    add_child(0, b, parse_link(c, has_matching_entity, cur));
                    break;

                case CMARK_NODE_HTML_BLOCK:
                case CMARK_NODE_HTML_INLINE:
                case CMARK_NODE_CUSTOM_BLOCK:
                case CMARK_NODE_CUSTOM_INLINE:
                case CMARK_NODE_IMAGE:
                case CMARK_NODE_FOOTNOTE_DEFINITION:
                case CMARK_NODE_FOOTNOTE_REFERENCE:
                    error(cur, std::string("forbidden CommonMark node of type \"")
                                   + cmark_node_get_type_string(cur) + "\"");
                    break;

                case CMARK_NODE_NONE:
                case CMARK_NODE_DOCUMENT:
                    assert(!static_cast<bool>("invalid node type"));
                    break;
                }
        }
    }
}

parse_result comment::parse(const parser& p, const std::string& comment, bool has_matching_entity)
{
    auto root = read_ast(p, comment);

    comment_builder builder;
    add_children(p.config(), builder, has_matching_entity, root.get());

    if (builder.brief || !builder.sections.empty() || !builder.data.is_empty())
        return parse_result{doc_comment(std::move(builder.data), std::move(builder.brief),
                                        std::move(builder.sections)),
                            std::move(builder.entity), std::move(builder.inlines)};
    else
        return parse_result{type_safe::nullopt, std::move(builder.entity),
                            std::move(builder.inlines)};
}
