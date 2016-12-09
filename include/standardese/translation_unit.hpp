// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_TRANSLATION_UNIT_HPP_INCLUDED
#define STANDARDESE_TRANSLATION_UNIT_HPP_INCLUDED

#include <standardese/detail/wrapper.hpp>
#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_entity_registry.hpp>

#include <iostream>

namespace standardese
{
    class parser;
    class compile_config;
    struct cpp_cursor;

    namespace detail
    {
        struct tu_deleter
        {
            void operator()(CXTranslationUnit tu) const STANDARDESE_NOEXCEPT
            {
                clang_disposeTranslationUnit(tu);
            }
        };

        using tu_wrapper = detail::wrapper<CXTranslationUnit, tu_deleter>;
    } // namespace detail

    class cpp_file : public cpp_entity, public cpp_entity_container<cpp_entity>
    {
    public:
        static cpp_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return cpp_entity::file_t;
        }

        void add_entity(cpp_entity_ptr e)
        {
            cpp_entity_container<cpp_entity>::add_entity(this, std::move(e));
        }

        cpp_name get_name() const override
        {
            return path_.c_str();
        }

        CXTranslationUnit get_cxunit() STANDARDESE_NOEXCEPT
        {
            return wrapper_.get();
        }

    private:
        cpp_file(cpp_name path)
        : cpp_entity(get_entity_type(), clang_getNullCursor()), path_(std::move(path))
        {
        }

        cpp_name           path_;
        detail::tu_wrapper wrapper_;

        friend parser;
    };

    class translation_unit
    {
    public:
        const parser& get_parser() const STANDARDESE_NOEXCEPT;

        const cpp_name& get_path() const STANDARDESE_NOEXCEPT;

        CXFile get_cxfile() const STANDARDESE_NOEXCEPT;

        cpp_file& get_file() STANDARDESE_NOEXCEPT;

        const cpp_file& get_file() const STANDARDESE_NOEXCEPT;

        CXTranslationUnit get_cxunit() const STANDARDESE_NOEXCEPT;

        const cpp_entity_registry& get_registry() const STANDARDESE_NOEXCEPT;

    private:
        translation_unit(const parser& par, const char* path, cpp_file* file);

        cpp_name      full_path_;
        cpp_file*     file_;
        const parser* parser_;

        friend parser;
    };

    namespace detail
    {
        template <typename Func>
        void visit_tu(CXTranslationUnit tu, const char* full_path, Func f)
        {
            struct data_t
            {
                Func*       func;
                const char* full_path;
            } data{&f, full_path};

            auto visitor_impl = [](CXCursor cursor, CXCursor parent,
                                   CXClientData client_data) -> CXChildVisitResult {
                auto data = static_cast<data_t*>(client_data);

                auto     location = clang_getCursorLocation(cursor);
                CXString cx_file_name;
                clang_getPresumedLocation(location, &cx_file_name, nullptr, nullptr);

                string file_name(cx_file_name);
                if (file_name != data->full_path)
                    return CXChildVisit_Continue;
                return (*data->func)(cursor, parent);
            };

            clang_visitChildren(clang_getTranslationUnitCursor(tu), visitor_impl, &data);
        }
    } // namespace detail
} // namespace standardese

#endif // STANDARDESE_TRANSLATION_UNIT_HPP_INCLUDED
