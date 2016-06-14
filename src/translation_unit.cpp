// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/translation_unit.hpp>

#include <standardese/detail/scope_stack.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/detail/wrapper.hpp>
#include <standardese/cpp_preprocessor.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

namespace
{
    struct tu_deleter
    {
        void operator()(CXTranslationUnit tu) const STANDARDESE_NOEXCEPT
        {
            clang_disposeTranslationUnit(tu);
        }
    };

    using tu_wrapper = detail::wrapper<CXTranslationUnit, tu_deleter>;
}

struct translation_unit::impl
{
    detail::context context;

    tu_wrapper tu;
    cpp_file*  file;

    const standardese::parser* parser;

    impl(const standardese::parser& p, CXTranslationUnit tunit, const char* path, cpp_file* file)
    : context(path), tu(tunit), file(file), parser(&p)
    {
        using namespace boost::wave;

        auto lang = support_cpp | support_option_variadics | support_option_long_long
                    | support_option_insert_whitespace | support_option_single_line;
        context.set_language(language_support(lang));
    }
};

// hidden function to get context from translation unit
detail::context& standardese::detail::get_preprocessing_context(translation_unit& tu)
{
    return tu.pimpl_->context;
}

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

cpp_name translation_unit::get_path() const STANDARDESE_NOEXCEPT
{
    return pimpl_->file->get_name();
}

CXFile translation_unit::get_cxfile() const STANDARDESE_NOEXCEPT
{
    auto file = clang_getFile(pimpl_->tu.get(), get_path().c_str());
    detail::validate(file);
    return file;
}

const cpp_file& translation_unit::get_file() const STANDARDESE_NOEXCEPT
{
    return *pimpl_->file;
}

CXTranslationUnit translation_unit::get_cxunit() const STANDARDESE_NOEXCEPT
{
    return pimpl_->tu.get();
}

const cpp_entity_registry& translation_unit::get_registry() const STANDARDESE_NOEXCEPT
{
    return pimpl_->parser->get_registry();
}

translation_unit::translation_unit(const parser& par, CXTranslationUnit tu, const char* path,
                                   cpp_file* file)
: pimpl_(new impl(par, tu, path, file))
{
    detail::scope_stack stack(pimpl_->file);

    detail::visit_tu(pimpl_->tu.get(), get_cxfile(), [&](cpp_cursor cur, cpp_cursor parent) {
        stack.pop_if_needed(parent);

        if (clang_getCursorSemanticParent(cur) != parent
            && clang_getCursorSemanticParent(cur) != cpp_cursor())
            // out of class definition, some weird other stuff with extern templates, implicit dtors
            return CXChildVisit_Continue;

        try
        {
            if (get_parser().get_logger()->level() <= spdlog::level::debug)
            {
                auto location = source_location(cur);
                get_parser()
                    .get_logger()
                    ->debug("Parsing entity '{}' of type '{}' at {}:{}",
                            string(clang_getCursorDisplayName(cur)).c_str(),
                            string(clang_getCursorKindSpelling(clang_getCursorKind(cur))).c_str(),
                            location.file_name, location.line);
            }

            if (clang_getCursorKind(cur) == CXCursor_MacroExpansion)
                pimpl_->context.add_macro_definition(detail::get_cmd_definition(cur));
            else
            {
                auto entity = cpp_entity::try_parse(*this, cur, stack.cur_parent());
                if (!entity)
                    return CXChildVisit_Continue;

                get_parser().get_registry().register_entity(*entity);

                auto container = stack.add_entity(std::move(entity), parent);
                if (container)
                    return CXChildVisit_Recurse;
            }

            return CXChildVisit_Continue;
        }
        catch (parse_error& ex)
        {
            if (ex.get_severity() == severity::warning)
                get_parser().get_logger()->warn("when parsing {} ({}:{}): {}",
                                                ex.get_location().entity_name,
                                                ex.get_location().file_name, ex.get_location().line,
                                                ex.what());
            else
                get_parser().get_logger()->error("when parsing {} ({}:{}): {}",
                                                 ex.get_location().entity_name,
                                                 ex.get_location().file_name,
                                                 ex.get_location().line, ex.what());
            return CXChildVisit_Continue;
        }
    });
}
