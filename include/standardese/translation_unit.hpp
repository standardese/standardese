// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_TRANSLATION_UNIT_HPP_INCLUDED
#define STANDARDESE_TRANSLATION_UNIT_HPP_INCLUDED

#include <clang-c/Index.h>
#include <string>

#include <standardese/detail/wrapper.hpp>
#include <standardese/cpp_entity.hpp>

namespace standardese
{
    class parser;

    class cpp_file
    : public cpp_entity, public cpp_entity_container<cpp_entity>
    {
    private:
        cpp_file(const char *name);

        friend class translation_unit;
    };

    namespace detail
    {
        // wrapper for the pimpl of translation_unit
        // this prevents writing move ctors for translation_unit
        struct context_impl
        {
            context_impl() STANDARDESE_NOEXCEPT = default;
            context_impl(context_impl &&other) STANDARDESE_NOEXCEPT;
            ~context_impl() STANDARDESE_NOEXCEPT;
            context_impl& operator=(context_impl &&other) STANDARDESE_NOEXCEPT;

            struct impl;
            std::unique_ptr<impl> pimpl;
        };

        context_impl& get_context_impl(translation_unit &tu);
    } // namespace detail

    class translation_unit
    {
    public:
        /// Calls given function for each entity in the current translation unit.
        template <typename Func>
        void visit(Func f) const
        {
            struct data_t
            {
                Func *func;
                CXFile file;
            } data{&f, get_cxfile()};

            auto visitor_impl = [](CXCursor cursor, CXCursor parent, CXClientData client_data) -> CXChildVisitResult
            {
                auto data = static_cast<data_t*>(client_data);

                auto location = clang_getCursorLocation(cursor);
                CXFile file;
                clang_getExpansionLocation(location, &file, nullptr, nullptr, nullptr);
                if (!file || !clang_File_isEqual(file, data->file))
                    return CXChildVisit_Continue;
                return (*data->func)(cursor, parent);
            };

            clang_visitChildren(clang_getTranslationUnitCursor(tu_.get()), visitor_impl, &data);
        }

        cpp_file& build_ast();

        const char* get_path() const STANDARDESE_NOEXCEPT
        {
            return path_.c_str();
        }

        CXFile get_cxfile() const STANDARDESE_NOEXCEPT;

        const parser& get_parser() const STANDARDESE_NOEXCEPT
        {
            return *parser_;
        }

    private:
        translation_unit(const parser &par, CXTranslationUnit tu, const char *path);

        class scope_stack;
        CXChildVisitResult parse_visit(scope_stack &stack, CXCursor cur, CXCursor parent);

        struct deleter
        {
            void operator()(CXTranslationUnit tu) const STANDARDESE_NOEXCEPT;
        };

        detail::wrapper<CXTranslationUnit, deleter> tu_;
        std::string path_;
        detail::context_impl impl_;
        const parser *parser_;

        friend parser;
        friend detail::context_impl& detail::get_context_impl(translation_unit &tu);
    };
} // namespace standardese

#endif // STANDARDESE_TRANSLATION_UNIT_HPP_INCLUDED
