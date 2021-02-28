// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cassert>
#include <algorithm>
#include <unordered_set>

#include <standardese/comment.hpp>

#include <cppast/cpp_friend.hpp>
#include <cppast/cpp_namespace.hpp>
#include <cppast/visitor.hpp>

#include "get_special_entity.hpp"

using namespace standardese;

void comment_registry::merge(comment_registry&& other)
{
    map_.insert(std::make_move_iterator(other.map_.begin()),
                std::make_move_iterator(other.map_.end()));
    groups_.insert(std::make_move_iterator(other.groups_.begin()),
                   std::make_move_iterator(other.groups_.end()));
    modules_.insert(std::make_move_iterator(other.modules_.begin()),
                    std::make_move_iterator(other.modules_.end()));
}

bool comment_registry::register_comment(type_safe::object_ref<const cppast::cpp_entity> entity,
                                        comment::doc_comment                            comment)
{
    auto iter = map_.find(&*entity);
    if (iter == map_.end())
        // not in map yet
        map_.emplace(&*entity, std::move(comment));
    else
    {
        auto& stored_comment = iter->second;
        if (stored_comment.brief_section() || !stored_comment.sections().empty())
            // already have a documentation
            return false;

        stored_comment = comment::merge(stored_comment.metadata(), std::move(comment));
    }

    return true;
}

bool comment_registry::register_comment(std::string module_name, comment::doc_comment comment)
{
    auto result = modules_.emplace(std::move(module_name), std::move(comment));
    return result.second;
}

type_safe::optional_ref<const comment::doc_comment> comment_registry::get_comment(
    const cppast::cpp_entity& e) const
{
    const cppast::cpp_entity* entity = &e;
    if (cppast::is_friended(*entity))
        entity = &entity->parent().value();
    if (cppast::is_templated(*entity))
        entity = &entity->parent().value();

    auto iter = map_.find(entity);
    if (iter == map_.end())
        return type_safe::nullopt;
    return type_safe::ref(iter->second);
}

type_safe::optional_ref<const comment::doc_comment> comment_registry::get_comment(
    const std::string& module_name) const
{
    auto iter = modules_.find(module_name);
    if (iter == modules_.end())
        return type_safe::nullopt;
    return type_safe::ref(iter->second);
}

namespace
{
cppast::source_location make_location(const cppast::cpp_entity&   entity,
                                      const comment::parse_error& ex)
{
    return cppast::source_location::make_entity(entity.name() + " - " + std::to_string(ex.line())
                                                + ":" + std::to_string(ex.column()));
}

cppast::diagnostic make_parse_diagnostic(const cppast::cpp_entity&   entity,
                                         const comment::parse_error& ex)
{
    return {ex.what(), make_location(entity, ex), cppast::severity::error};
}

template <typename... Args>
cppast::diagnostic make_semantic_diagnostic(const cppast::cpp_entity& entity, Args&&... args)
{
    return make_diagnostic(cppast::source_location::make_entity(entity.name()),
                           std::forward<Args>(args)...);
}
template <class Inline>
type_safe::optional<comment::unmatched_doc_comment> find_inline(comment::parse_result& result,
                                                                const Inline&          entity)
{
    auto iter = std::find_if(result.inlines.begin(), result.inlines.end(),
                             [&](const comment::unmatched_doc_comment& comment) {
                                 return comment.entity == entity;
                             });
    if (iter == result.inlines.end())
        return type_safe::nullopt;

    auto comment = std::move(*iter);
    result.inlines.erase(iter);
    return std::move(comment);
}

template <class Inline, class InlineContainer, typename MatchRegister, typename UnmatchRegister>
void process_inlines(type_safe::optional<comment::parse_result>& comment,
                     const InlineContainer& container, const MatchRegister& register_commented,
                     const UnmatchRegister& register_uncommented)
{
    auto cur = 0;
    for (auto& child : container)
    {
        // create corresponding matching entity
        Inline name(child.name().empty() ? std::to_string(cur) : child.name());
        auto   inline_comment = comment.map(
            [&](comment::parse_result& comment) { return find_inline(comment, name); });

        // register
        if (inline_comment)
            register_commented(type_safe::ref(child), std::move(inline_comment.value().comment));
        else
            register_uncommented(type_safe::ref(child));

        ++cur;
    }
}

template <typename MatchRegister, typename UnmatchRegister>
void process_inlines(const cppast::diagnostic_logger&            logger,
                     type_safe::optional<comment::parse_result>& comment,
                     const cppast::cpp_entity& entity, const MatchRegister& register_commented,
                     const UnmatchRegister& register_uncommented)
{
    if (auto macro = detail::get_macro(entity))
        process_inlines<comment::inline_param>(comment, macro.value().parameters(),
                                               register_commented, register_uncommented);
    if (auto func = detail::get_function(entity))
        process_inlines<comment::inline_param>(comment, func.value().parameters(),
                                               register_commented, register_uncommented);
    if (auto templ = detail::get_template(entity))
        process_inlines<comment::inline_param>(comment, templ.value().parameters(),
                                               register_commented, register_uncommented);
    if (auto c = detail::get_class(entity))
        process_inlines<comment::inline_base>(comment, c.value().bases(), register_commented,
                                              register_uncommented);

    // error on remaining inlines
    if (comment)
        for (auto& inl : comment.value().inlines)
        {
            if (auto param = comment::get_inline_param(inl.entity))
                logger.log("standardese comment",
                           make_semantic_diagnostic(entity,
                                                    "unable to find function or "
                                                    "template parameter named '",
                                                    param.value(), "'"));
            else if (auto base = comment::get_inline_base(inl.entity))
                logger.log("standardese comment",
                           make_semantic_diagnostic(entity, "unable to find base named '",
                                                    base.value(), "'"));
            else
                logger.log("standardese comment",
                           make_semantic_diagnostic(entity, "unexpected inline comment"));
        }
}
} // namespace

void file_comment_parser::parse(type_safe::object_ref<const cppast::cpp_file> file) const
{
    // add matched comments
    cppast::visit(*file, [&](const cppast::cpp_entity& entity, const cppast::visitor_info& info) {
        if (info.event == cppast::visitor_info::container_entity_exit)
            // entity already handled
            return true;
        else if (!cppast::is_templated(entity) && !cppast::is_friended(entity))
        {
            auto register_commented = [&](type_safe::object_ref<const cppast::cpp_entity> e,
                                          comment::doc_comment                            comment) {
                this->register_commented(e, std::move(comment));
            };
            auto register_uncommented = [&](type_safe::object_ref<const cppast::cpp_entity> e) {
                this->register_uncommented(e);
            };

            // parse comment
            type_safe::optional<comment::parse_result> comment;
            try
            {
                comment = type_safe::copy(entity.comment()).map([&](const std::string& str) {
                    return comment::parse(comment::parser(config_), str, true);
                });
            }
            catch (comment::parse_error& ex)
            {
                logger_->log("standardese comment", make_parse_diagnostic(entity, ex));
                comment
                    = comment::parse_result{comment::doc_comment(comment::metadata(),
                                                                 markup::brief_section::builder()
                                                                     .add_child(markup::text::build(
                                                                         std::string(
                                                                             "(error while parsing "
                                                                             "comment text: ")
                                                                         + ex.what() + ")"))
                                                                     .finish(),
                                                                 {}),
                                            type_safe::nullvar,
                                            {}};
            }

            if (comment && comment.value().comment)
                // register comment
                register_commented(type_safe::ref(entity),
                                   std::move(comment.value().comment.value()));
            else
                register_uncommented(type_safe::ref(entity));

            process_inlines(*logger_, comment, entity, register_commented, register_uncommented);
        }

        return true;
    });

    // add free comments
    for (auto& free : file->unmatched_comments())
    {
        const auto log = [&](auto... message)
        {
            logger_->log("standardese comment",
              make_diagnostic(cppast::source_location::make_file(file->name(),free.line),
              message...));
        };

        auto comment = comment::parse(comment::parser(config_), free.content, false);
        if (auto module = comment::get_module(comment.entity))
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (!registry_.register_comment(module.value(), std::move(comment.comment.value())))
                log("multiple comments for module '", module.value(), "'");
        }
        else if (auto name = comment::get_remote_entity(comment.entity))
        {
            std::unique_lock<std::mutex> lock(mutex_);
            free_comments_.push_back(std::move(comment));
        }
        else if (comment::is_file(comment.entity) || config_.free_file_comments())
        {
            // comment for current file
            if (!register_commented(file, std::move(comment.comment.value())))
                log("multiple file comments");
        }
        else
        {
            log("comment does not have a remote entity specified");
        }
    }
}

comment_registry file_comment_parser::finish()
{
    resolve_free_comments();
    if (config_.group_uncommented())
        group_uncommented();
    return std::move(registry_);
}

void file_comment_parser::resolve_free_comments()
{
    // Attach comments that are using the `\entity` command to the entity they're documenting.
    for (auto& free : free_comments_)
    {
        // Find all the entities that are not documented yet that match this entity command.
        auto result = uncommented_.equal_range(comment::get_remote_entity(free.entity).value());
        if (result.first != result.second)
        {
            auto metadata = free.comment.value().metadata();

            // Assign the entire comment block to the first entity found.
            register_commented(type_safe::ref(*result.first->second),
                               std::move(free.comment.value()), false);

            // And only the metadata to all the other entities found.
            // TODO: What is an example where this actually happens? This does not show up in our test cases.
            for (auto cur = std::next(result.first); cur != result.second; ++cur)
                register_commented(type_safe::ref(*cur->second),
                                   comment::doc_comment(metadata, nullptr, {}), false);

            uncommented_.erase(result.first, result.second);
        }
        else
            logger_->log("standardese comment",
                         make_diagnostic(cppast::source_location(),
                                         "unable to find matching undocumented entity '",
                                         comment::get_remote_entity(free.entity).value(),
                                         "' for comment"));
    }
}

void file_comment_parser::group_uncommented()
{
    // Add undocumented members to the group their preceding member is in.
    std::unordered_set<const cppast::cpp_file*> files;
    for (const auto& uncommented : uncommented_) {
        const cppast::cpp_entity* file = uncommented.second;
        while (file->parent())
            file = &file->parent().value();

        assert(file->kind() == cppast::cpp_entity_kind::file_t && "all entities must live under a file root node");

        files.insert(static_cast<const cppast::cpp_file*>(file));
    }

    for (const auto* file : files) {
        std::stack<const cppast::cpp_entity*> previous;

        previous.push(nullptr);

        const auto assign_group = [&](const cppast::cpp_entity& target, const cppast::cpp_entity* source) {
            if (source == nullptr)
                return;

            switch (target.kind()) {
                case cppast::cpp_entity_kind::enum_value_t:
                case cppast::cpp_entity_kind::function_t:
                case cppast::cpp_entity_kind::function_template_t:
                case cppast::cpp_entity_kind::member_function_t:
                case cppast::cpp_entity_kind::conversion_op_t:
                case cppast::cpp_entity_kind::constructor_t:
                    break;
                case cppast::cpp_entity_kind::destructor_t:
                    // Do not automatically group the destructor as it
                    // typically undocumented and the intention was probably
                    // just to exclude it from the output.
                    [[fallthrough]];
                case cppast::cpp_entity_kind::enum_t:
                case cppast::cpp_entity_kind::class_template_t:
                case cppast::cpp_entity_kind::class_t:
                    // Do not group types automatically as this is usually not
                    // what users expect. Instead, uncommented (inner) types
                    // were probably meant to be hidden from the output.
                    [[fallthrough]];
                case cppast::cpp_entity_kind::member_variable_t:
                case cppast::cpp_entity_kind::file_t:
                case cppast::cpp_entity_kind::macro_parameter_t:
                case cppast::cpp_entity_kind::macro_definition_t:
                case cppast::cpp_entity_kind::include_directive_t:
                case cppast::cpp_entity_kind::language_linkage_t:
                case cppast::cpp_entity_kind::namespace_t:
                case cppast::cpp_entity_kind::namespace_alias_t:
                case cppast::cpp_entity_kind::using_directive_t:
                case cppast::cpp_entity_kind::using_declaration_t:
                case cppast::cpp_entity_kind::type_alias_t:
                case cppast::cpp_entity_kind::access_specifier_t:
                case cppast::cpp_entity_kind::base_class_t:
                case cppast::cpp_entity_kind::variable_t:
                case cppast::cpp_entity_kind::bitfield_t:
                case cppast::cpp_entity_kind::function_parameter_t:
                case cppast::cpp_entity_kind::friend_t:
                case cppast::cpp_entity_kind::template_type_parameter_t:
                case cppast::cpp_entity_kind::non_type_template_parameter_t:
                case cppast::cpp_entity_kind::template_template_parameter_t:
                case cppast::cpp_entity_kind::alias_template_t:
                case cppast::cpp_entity_kind::variable_template_t:
                case cppast::cpp_entity_kind::function_template_specialization_t:
                case cppast::cpp_entity_kind::class_template_specialization_t:
                case cppast::cpp_entity_kind::static_assert_t:
                    // Do not implicitly group things that people usually
                    // don't want to be grouped or that we do not generate comments for anyway.
                    [[fallthrough]];
                default:
                    return;
            }

            auto target_comment = registry_.get_comment(target);

            if (target_comment.has_value() && target_comment.value().metadata().group())
                // Do not implicitly assign a group if this member already has one.
                return;

            if (target_comment.has_value() && (target_comment.value().brief_section().has_value() || !target_comment.value().sections().empty()))
                // Do not implicitly assign a group if this member already has some comment.
                return;

            const auto source_comment = registry_.get_comment(*source);

            if (!source_comment.has_value() || !source_comment.value().metadata().group().has_value())
                // Source has no group so we cannot assign it to target.
                return;

            comment::metadata metadata;
            metadata.set_group(source_comment.value().metadata().group().value());

            register_commented(type_safe::ref(target), comment::doc_comment(metadata, nullptr, {}), false);
        };

        cppast::visit(*file, [&](const cppast::cpp_entity& entity, const cppast::visitor_info& info) {
            switch(info.event) {
                case cppast::visitor_info::container_entity_enter:
                    previous.push(nullptr);
                    break;
                case cppast::visitor_info::container_entity_exit:
                    previous.pop();
                    [[fallthrough]];
                case cppast::visitor_info::leaf_entity:
                    assign_group(entity, previous.top());
                    previous.pop();
                    previous.push(&entity);
                    break;
                default:
                    throw std::logic_error("not implemented: unknown event while grouping entities implicitly");
            }
        });

        assert(previous.size() == 1 && "stack inconsistent; expected the stack to be in the original 'empty' state");
    }
}

bool file_comment_parser::register_commented(type_safe::object_ref<const cppast::cpp_entity> entity,
                                             comment::doc_comment comment, bool allow_cmd) const
{
    auto cmd_comment = !comment.brief_section() && comment.sections().empty();

    std::lock_guard<std::mutex> lock(mutex_);
    if (comment.metadata().group())
        registry_.add_to_group(comment.metadata().group().value().name(), entity);
    auto result = registry_.register_comment(entity, std::move(comment));

    if (cmd_comment && allow_cmd)
        // a pure "command" comment, allow later sections
        uncommented_.emplace(lookup_unique_name(registry_, *entity), &*entity);

    return result;
}

namespace
{
bool is_relative_unique_name(const std::string& unique_name)
{
    return unique_name.front() == '*' || unique_name.front() == '?';
}

std::string get_template_parameters(const cppast::cpp_entity& e)
{
    auto templ = detail::get_template(e);
    if (!templ)
        return "";

    std::string result = "<";
    for (auto& param : templ.value().parameters())
        result += param.name() + ",";
    result.back() = '>';

    return result;
}

std::string get_signature(const cppast::cpp_entity& e)
{
    auto function = detail::get_function(e);
    if (!function)
        return "";
    return function.value().signature();
}

// get unique name of entity only w/o parent
std::string get_unique_name(const cppast::cpp_entity& e)
{
    if (e.kind() == cppast::cpp_file::kind())
    {
        auto index = e.name().find_last_of("/\\");
        assert(index != e.name().size());
        if (index != std::string::npos)
            return e.name().substr(index + 1u);
        else
            return e.name();
    }
    else if (e.kind() == cppast::cpp_friend::kind())
    {
        auto& f = static_cast<const cppast::cpp_friend&>(e);
        if (f.entity())
            return get_unique_name(f.entity().value());
        else
            return "";
    }
    else if (e.name().empty() && cppast::is_parameter(e.kind()))
    {
        auto& parent = e.parent().value();

        // find index and use it as name
        if (auto func = detail::get_function(parent))
        {
            auto count = 0u;
            for (auto& param : func.value().parameters())
            {
                if (&param == &e)
                    return std::to_string(count);
                ++count;
            }
        }

        if (auto templ = detail::get_template(parent))
        {
            auto count = 0u;
            for (auto& param : templ.value().parameters())
            {
                if (&param == &e)
                    return std::to_string(count);
                ++count;
            }
        }

        assert(false);
        return "";
    }
    else
    {
        auto result = e.name();
        result += get_template_parameters(e);
        result += get_signature(e);
        return result;
    }
}

std::string get_separator(const cppast::cpp_entity& e)
{
    if (cppast::is_parameter(e.kind()))
        return ".";
    else
        return "::";
}

std::string get_full_unique_name(const std::string& parent, const cppast::cpp_entity& e,
                                 const std::string& e_name)
{
    std::string result = parent;
    if (e_name.empty())
        return result;
    else if (!(parent.empty() || parent.back() == ':' || parent.back() == '.'
               || e_name.front() == ':' || e_name.front() == '.'))
        result += get_separator(e);

    for (auto c : e_name)
        if (c != ' ')
            result += c;

    return result;
}

template <class Lookup>
std::string lookup_parent_unique_name(const Lookup& get_comment, const cppast::cpp_entity& e)
{
    auto parent = e.parent();
    while (parent && (cppast::is_templated(parent.value()) || cppast::is_friended(parent.value())))
        parent = parent.value().parent();
    if (!parent)
        return "";

    // don't need unique name for parents that don't have a scope
    // except for functions or templates, those are fine
    auto need_name = parent.value().scope_name() || detail::get_function(parent.value())
                     || detail::get_template(parent.value());
    if (!need_name)
        return "";

    auto result
        = parent
              .map([&](const cppast::cpp_entity& p) {
                  return p.scope_name() || detail::get_function(p) ? get_comment(p)
                                                                   : type_safe::nullopt;
              })
              .map([](const comment::doc_comment& c) { return c.metadata().unique_name(); });
    if (result)
        return result.value();

    // parent doesn't have a unique name
    return get_full_unique_name(lookup_parent_unique_name(get_comment, parent.value()),
                                parent.value(), get_unique_name(parent.value()));
}
} // namespace

void file_comment_parser::register_uncommented(
    type_safe::object_ref<const cppast::cpp_entity> entity) const
{
    auto unique_name
        = get_full_unique_name(get_parent_unique_name(*entity), *entity, get_unique_name(*entity));

    std::lock_guard<std::mutex> lock(mutex_);
    uncommented_.emplace(std::move(unique_name), &*entity);
}

std::string file_comment_parser::get_parent_unique_name(const cppast::cpp_entity& e) const
{
    return lookup_parent_unique_name([&](const cppast::cpp_entity& e) { return get_comment(e); },
                                     e);
}

std::string standardese::lookup_unique_name(const comment_registry&   registry,
                                            const cppast::cpp_entity& e)
{
    auto comment = registry.get_comment(e);
    if (comment && comment.value().metadata().unique_name())
    {
        if (is_relative_unique_name(comment.value().metadata().unique_name().value()))
        {
            auto parent = lookup_parent_unique_name([&](const cppast::cpp_entity&
                                                            e) { return registry.get_comment(e); },
                                                    e);
            return get_full_unique_name(parent, e,
                                        comment.value().metadata().unique_name().value().substr(1));
        }
        else
            return comment.value().metadata().unique_name().value();
    }

    // calculate unique name
    auto parent = lookup_parent_unique_name([&](const cppast::cpp_entity&
                                                    e) { return registry.get_comment(e); },
                                            e);
    return get_full_unique_name(parent, e, get_unique_name(e));
}
