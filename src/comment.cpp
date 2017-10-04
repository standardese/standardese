// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/comment.hpp>

#include <cppast/cpp_class.hpp>
#include <cppast/cpp_file.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_template.hpp>
#include <cppast/visitor.hpp>

#include <algorithm>

using namespace standardese;

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

namespace
{
    cppast::source_location make_location(const cppast::cpp_entity&   entity,
                                          const comment::parse_error& ex)
    {
        return cppast::source_location::make_entity(
            entity.name() + " - " + std::to_string(ex.line()) + ":" + std::to_string(ex.column()));
    }

    cppast::diagnostic make_parse_diagnostic(const cppast::cpp_entity&   entity,
                                             const comment::parse_error& ex)
    {
        return make_diagnostic(make_location(entity, ex), ex.what());
    }

    template <typename... Args>
    cppast::diagnostic make_semantic_diagnostic(const cppast::cpp_entity& entity, Args&&... args)
    {
        return make_diagnostic(cppast::source_location::make_entity(entity.name()),
                               std::forward<Args>(args)...);
    }

    type_safe::optional_ref<const cppast::cpp_function_base> get_function(
        const cppast::cpp_entity& e)
    {
        if (cppast::is_function(e.kind()))
            return type_safe::ref(static_cast<const cppast::cpp_function_base&>(e));
        else if (cppast::is_template(e.kind()))
            return get_function(*static_cast<const cppast::cpp_template&>(e).begin());
        else
            return type_safe::nullopt;
    }

    type_safe::optional_ref<const cppast::cpp_template> get_template(const cppast::cpp_entity& e)
    {
        if (cppast::is_template(e.kind()))
            return type_safe::ref(static_cast<const cppast::cpp_template&>(e));
        else if (cppast::is_templated(e))
            return get_template(e.parent().value());
        else
            return type_safe::nullopt;
    }

    type_safe::optional_ref<const cppast::cpp_class> get_class(const cppast::cpp_entity& e)
    {
        if (e.kind() == cppast::cpp_class::kind())
            return type_safe::ref(static_cast<const cppast::cpp_class&>(e));
        else if (cppast::is_template(e.kind()))
            return get_class(*static_cast<const cppast::cpp_template&>(e).begin());
        else
            return type_safe::nullopt;
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
                register_commented(type_safe::ref(child),
                                   std::move(inline_comment.value().comment));
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
        if (auto func = get_function(entity))
            process_inlines<comment::inline_param>(comment, func.value().parameters(),
                                                   register_commented, register_uncommented);
        if (auto templ = get_template(entity))
            process_inlines<comment::inline_param>(comment, templ.value().parameters(),
                                                   register_commented, register_uncommented);
        if (auto c = get_class(entity))
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
}

void file_comment_parser::parse(type_safe::object_ref<const cppast::cpp_file> file) const
{
    comment::parser p(config_);

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
            try
            {
                auto comment = type_safe::copy(entity.comment()).map([&](const std::string& str) {
                    return comment::parse(p, str);
                });

                if (comment)
                    // register comment
                    // important to do this before the inlines, as parent needs to be processed before the child,
                    // for the unqiue name calculation
                    register_commented(type_safe::ref(entity), std::move(comment.value().comment));
                else
                    register_uncommented(type_safe::ref(entity));

                process_inlines(*logger_, comment, entity, register_commented,
                                register_uncommented);
            }
            catch (comment::parse_error& ex)
            {
                logger_->log("standardese comment", make_parse_diagnostic(entity, ex));
            }
        }

        return true;
    });

    // add free comments
    for (auto& free : file->unmatched_comments())
    {
        auto comment = comment::parse(p, free);
        if (comment::is_file(comment.entity))
        {
            // comment for current file
            if (!register_commented(file, std::move(comment.comment)))
                logger_->log("standardese comment",
                             make_semantic_diagnostic(*file, "multiple file comments"));
        }
        else if (auto name = comment::get_remote_entity(comment.entity))
            free_comments_.push_back(std::move(comment));
        else
            logger_->log("standardese comment",
                         make_semantic_diagnostic(*file, "unmatched comment doesn't have a remote "
                                                         "entity specified"));
    }
}

comment_registry file_comment_parser::finish()
{
    for (auto& free : free_comments_)
    {
        auto iter = uncommented_.find(comment::get_remote_entity(free.entity).value());
        if (iter != uncommented_.end())
        {
            register_commented(type_safe::ref(*iter->second), std::move(free.comment));
            uncommented_.erase(iter);
        }
        else
            logger_->log("standardese comment",
                         make_diagnostic(cppast::source_location(),
                                         "unable to find matching entity '",
                                         comment::get_remote_entity(free.entity).value(),
                                         "' for comment"));
    }

    return std::move(registry_);
}

namespace
{
    bool is_relative_unique_name(const std::string& unique_name)
    {
        return unique_name.front() == '*' || unique_name.front() == '?';
    }

    std::string get_template_parameters(const cppast::cpp_entity& e)
    {
        auto templ = get_template(e);
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
        auto function = get_function(e);
        if (!function)
            return "";
        return function.value().signature();
    }

    // get unique name of entity only w/o parent
    std::string get_unique_name(const cppast::cpp_entity& e)
    {
        auto result = e.name();
        result += get_template_parameters(e);
        result += get_signature(e);
        return result;
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

        if (!(parent.empty() || parent.back() == ':' || parent.back() == '.'
              || e_name.front() == ':' || e_name.front() == '.'))
            result += get_separator(e);

        for (auto c : e_name)
            if (c != ' ')
                result += c;

        return result;
    }
}

bool file_comment_parser::register_commented(type_safe::object_ref<const cppast::cpp_entity> entity,
                                             comment::doc_comment comment) const
{
    // set unique name
    auto& unique_name = comment.metadata().unique_name();
    if (!unique_name)
    {
        // need to create one
        auto parent = get_parent_unique_name(*entity);
        comment.metadata().set_unique_name(
            get_full_unique_name(parent, *entity, get_unique_name(*entity)));
    }
    else if (is_relative_unique_name(unique_name.value()))
    {
        // need to add parent unique name
        auto parent = get_parent_unique_name(*entity);
        comment.metadata().set_unique_name(
            get_full_unique_name(parent, *entity, unique_name.value()));
    }

    if (!comment.brief_section() && comment.sections().empty())
    {
        // a pure "command" comment, allow later sections
        std::lock_guard<std::mutex> lock(mutex_);
        uncommented_.emplace(unique_name.value(), &*entity);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (comment.metadata().group())
        registry_.add_to_group(comment.metadata().group().value().name(), entity);
    return registry_.register_comment(entity, std::move(comment));
}

void file_comment_parser::register_uncommented(
    type_safe::object_ref<const cppast::cpp_entity> entity) const
{
    auto unique_name =
        get_full_unique_name(get_parent_unique_name(*entity), *entity, get_unique_name(*entity));

    std::lock_guard<std::mutex> lock(mutex_);
    uncommented_.emplace(std::move(unique_name), &*entity);
}

std::string file_comment_parser::get_parent_unique_name(const cppast::cpp_entity& e) const
{
    auto parent = e.parent();
    while (parent && (cppast::is_templated(parent.value()) || cppast::is_friended(parent.value())))
        parent = parent.value().parent();
    if (!parent)
        return "";

    // don't need unique name for parents that don't have a scope
    // except for functions, those are fine
    auto need_name = parent.value().scope_name() || get_function(parent.value());
    if (!need_name)
        return "";

    auto result =
        parent
            .map([&](const cppast::cpp_entity& p) {
                return p.scope_name() || get_function(p) ? get_comment(p) : type_safe::nullopt;
            })
            .map([](const comment::doc_comment& c) { return c.metadata().unique_name(); });
    if (result)
        return result.value();

    // parent doesn't have a unique name yet
    return get_full_unique_name(get_parent_unique_name(parent.value()), parent.value(),
                                get_unique_name(parent.value()));
}
