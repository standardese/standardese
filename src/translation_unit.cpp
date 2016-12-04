// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/translation_unit.hpp>

#include <fstream>
#include <iterator>
#include <string>

#include <standardese/detail/scope_stack.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/detail/wrapper.hpp>
#include <standardese/comment.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/error.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

const parser& translation_unit::get_parser() const STANDARDESE_NOEXCEPT
{
    return *parser_;
}

const cpp_name& translation_unit::get_path() const STANDARDESE_NOEXCEPT
{
    return full_path_;
}

CXFile translation_unit::get_cxfile() const STANDARDESE_NOEXCEPT
{
    auto file = clang_getFile(get_cxunit(), get_path().c_str());
    detail::validate(file);
    return file;
}

cpp_file& translation_unit::get_file() STANDARDESE_NOEXCEPT
{
    return *file_;
}

const cpp_file& translation_unit::get_file() const STANDARDESE_NOEXCEPT
{
    return *file_;
}

CXTranslationUnit translation_unit::get_cxunit() const STANDARDESE_NOEXCEPT
{
    return file_->get_cxunit();
}

const cpp_entity_registry& translation_unit::get_registry() const STANDARDESE_NOEXCEPT
{
    return parser_->get_entity_registry();
}

namespace
{
    bool handle_cursor(cpp_cursor cur)
    {
        auto kind = clang_getCursorKind(cur);
        if (kind == CXCursor_StructDecl || kind == CXCursor_ClassDecl || kind == CXCursor_UnionDecl
            || kind == CXCursor_EnumDecl || kind == CXCursor_ClassTemplate
            || kind == CXCursor_ClassTemplatePartialSpecialization)
            // handle those if this is a definition
            return clang_isCursorDefinition(cur) != 0;

        // handle the rest all the time
        return true;
    }
}

translation_unit::translation_unit(const parser& par, const char* path, cpp_file* file)
: full_path_(path), file_(file), parser_(&par)
{
    detail::scope_stack stack(file_);

    detail::visit_tu(get_cxunit(), get_cxfile(), [&](cpp_cursor cur, cpp_cursor parent) {
        stack.pop_if_needed(parent);

        if (clang_getCursorSemanticParent(cur) != parent
            && clang_getCursorSemanticParent(cur) != cpp_cursor())
            // out of class definition, some weird other stuff with extern templates, implicit dtors
            return CXChildVisit_Continue;

        try
        {
            if (!handle_cursor(cur))
                return CXChildVisit_Continue;
            else if (get_parser().get_logger()->level() <= spdlog::level::debug)
            {
                auto location = source_location(cur);
                get_parser()
                    .get_logger()
                    ->debug("parsing entity '{}' ({}:{}) of type '{}'",
                            string(clang_getCursorDisplayName(cur)).c_str(), location.file_name,
                            location.line,
                            string(clang_getCursorKindSpelling(clang_getCursorKind(cur))).c_str());
            }

            auto entity = cpp_entity::try_parse(*this, cur, stack.cur_parent());
            if (!entity)
                return CXChildVisit_Continue;

            get_parser().get_entity_registry().register_entity(*entity);

            auto container = stack.add_entity(std::move(entity), parent);
            if (container)
                return CXChildVisit_Recurse;

            return CXChildVisit_Continue;
        }
        catch (parse_error& ex)
        {
            if (ex.get_severity() == severity::warning)
                get_parser().get_logger()->warn("when parsing '{}' ({}:{}): {}",
                                                ex.get_location().entity_name,
                                                ex.get_location().file_name, ex.get_location().line,
                                                ex.what());
            else
                get_parser().get_logger()->error("when parsing '{}' ({}:{}): {}",
                                                 ex.get_location().entity_name,
                                                 ex.get_location().file_name,
                                                 ex.get_location().line, ex.what());
            return CXChildVisit_Continue;
        }
    });
}
