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
#include <standardese/cpp_preprocessor.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/error.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

struct translation_unit::impl
{
    cpp_name                   full_path;
    cpp_file*                  file;
    const standardese::parser* parser;

    impl(const standardese::parser& p, const char* path, cpp_file* file)
    : full_path(path), file(file), parser(&p)
    {
        // need to open in binary mode as libclang does it (apparently)
        // otherwise the offsets are incompatible under Windows
        std::filebuf filebuf;
        filebuf.open(path, std::ios_base::in | std::ios_base::binary);
        assert(filebuf.is_open());

        auto source =
            std::string(std::istreambuf_iterator<char>(&filebuf), std::istreambuf_iterator<char>());
        if (source.back() != '\n')
            source.push_back('\n');

        parse_comments(p, full_path, source);
    }
};

translation_unit::translation_unit(translation_unit&& other) STANDARDESE_NOEXCEPT
    : pimpl_(std::move(other.pimpl_))
{
}

translation_unit::~translation_unit() STANDARDESE_NOEXCEPT
{
}

translation_unit& translation_unit::operator=(translation_unit&& other) STANDARDESE_NOEXCEPT
{
    pimpl_ = std::move(other.pimpl_);
    return *this;
}

const parser& translation_unit::get_parser() const STANDARDESE_NOEXCEPT
{
    return *pimpl_->parser;
}

const cpp_name& translation_unit::get_path() const STANDARDESE_NOEXCEPT
{
    return pimpl_->full_path;
}

CXFile translation_unit::get_cxfile() const STANDARDESE_NOEXCEPT
{
    auto file = clang_getFile(get_cxunit(), get_path().c_str());
    detail::validate(file);
    return file;
}

cpp_file& translation_unit::get_file() STANDARDESE_NOEXCEPT
{
    return *pimpl_->file;
}

const cpp_file& translation_unit::get_file() const STANDARDESE_NOEXCEPT
{
    return *pimpl_->file;
}

CXTranslationUnit translation_unit::get_cxunit() const STANDARDESE_NOEXCEPT
{
    return pimpl_->file->get_cxunit();
}

const cpp_entity_registry& translation_unit::get_registry() const STANDARDESE_NOEXCEPT
{
    return pimpl_->parser->get_entity_registry();
}

translation_unit::translation_unit(const parser& par, const char* path, cpp_file* file,
                                   const compile_config& config)
: pimpl_(new impl(par, path, file))
{
    detail::scope_stack stack(pimpl_->file);

    detail::visit_tu(get_cxunit(), get_cxfile(),
                     [&](cpp_cursor cur, cpp_cursor parent) {
                         stack.pop_if_needed(parent);

                         if (clang_getCursorSemanticParent(cur) != parent
                             && clang_getCursorSemanticParent(cur) != cpp_cursor())
                             // out of class definition, some weird other stuff with extern templates, implicit dtors
                             return CXChildVisit_Continue;

                         try
                         {
                             if (clang_getCursorKind(cur) == CXCursor_Namespace
                                 || clang_getCursorKind(cur) == CXCursor_LinkageSpec
                                 || is_full_specialization(*this, cur)
                                 || cur == clang_getCanonicalCursor(
                                               cur)) // only parse the canonical cursor
                             {
                                 if (get_parser().get_logger()->level() <= spdlog::level::debug)
                                 {
                                     auto location = source_location(cur);
                                     get_parser()
                                         .get_logger()
                                         ->debug("parsing entity '{}' ({}:{}) of type '{}'",
                                                 string(clang_getCursorDisplayName(cur)).c_str(),
                                                 location.file_name, location.line,
                                                 string(clang_getCursorKindSpelling(
                                                            clang_getCursorKind(cur)))
                                                     .c_str());
                                 }

                                 auto entity =
                                     cpp_entity::try_parse(*this, cur, stack.cur_parent());
                                 if (!entity)
                                     return CXChildVisit_Continue;

                                 get_parser().get_entity_registry().register_entity(*entity);

                                 auto container = stack.add_entity(std::move(entity), parent);
                                 if (container)
                                     return CXChildVisit_Recurse;
                             }
                             else
                             {
                                 if (get_parser().get_logger()->level() <= spdlog::level::debug)
                                 {
                                     auto location = source_location(cur);
                                     get_parser()
                                         .get_logger()
                                         ->debug("rejected entity '{}' ({}:{}) of type '{}'",
                                                 string(clang_getCursorDisplayName(cur)).c_str(),
                                                 location.file_name, location.line,
                                                 string(clang_getCursorKindSpelling(
                                                            clang_getCursorKind(cur)))
                                                     .c_str());
                                 }

                                 get_parser().get_entity_registry().register_alternative(cur);
                             }

                             return CXChildVisit_Continue;
                         }
                         catch (parse_error& ex)
                         {
                             if (ex.get_severity() == severity::warning)
                                 get_parser().get_logger()->warn("when parsing '{}' ({}:{}): {}",
                                                                 ex.get_location().entity_name,
                                                                 ex.get_location().file_name,
                                                                 ex.get_location().line, ex.what());
                             else
                                 get_parser().get_logger()->error("when parsing '{}' ({}:{}): {}",
                                                                  ex.get_location().entity_name,
                                                                  ex.get_location().file_name,
                                                                  ex.get_location().line,
                                                                  ex.what());
                             return CXChildVisit_Continue;
                         }
                     },
                     [&](cpp_cursor macro) {});
}
