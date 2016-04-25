// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_TRANSLATION_UNIT_HPP_INCLUDED
#define STANDARDESE_TRANSLATION_UNIT_HPP_INCLUDED

#include <clang-c/Index.h>
#include <string>

#include <standardese/detail/wrapper.hpp>

namespace standardese
{
    class translation_unit
    {
    public:
        /// Calls given function for each entity in the current translation unit.
        template <typename Func>
        void visit(Func f)
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

        const char* get_path() const STANDARDESE_NOEXCEPT
        {
            return path_.c_str();
        }

        CXFile get_cxfile() const STANDARDESE_NOEXCEPT;

    private:
        translation_unit(CXTranslationUnit tu, const char *path);

        struct deleter
        {
            void operator()(CXTranslationUnit tu) const STANDARDESE_NOEXCEPT;
        };

        detail::wrapper<CXTranslationUnit, deleter> tu_;
        std::string path_;

        friend class parser;
    };
} // namespace standardese

#endif // STANDARDESE_TRANSLATION_UNIT_HPP_INCLUDED
