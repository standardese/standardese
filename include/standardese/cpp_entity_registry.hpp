// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_ENTITY_REGISTRY_HPP_INCLUDED
#define STANDARDESE_CPP_ENTITY_REGISTRY_HPP_INCLUDED

#include <map>
#include <mutex>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/cpp_entity.hpp>

namespace standardese
{
    class cpp_entity_registry
    {
    public:
        void register_entity(const cpp_entity& e) const
        {
            std::unique_lock<std::mutex> lock(mutex_);
            map_.emplace(e.get_cursor(), &e);
        }

        const cpp_entity& lookup_entity(const cpp_cursor& cur) const
        {
            std::unique_lock<std::mutex> lock(mutex_);
            return *map_.at(cur);
        }

        const cpp_entity* try_lookup(const cpp_cursor& cur) const STANDARDESE_NOEXCEPT
        {
            std::unique_lock<std::mutex> lock(mutex_);
            auto                         iter = map_.find(cur);
            return iter == map_.end() ? nullptr : iter->second;
        }

    private:
        mutable std::mutex mutex_;
        mutable std::map<cpp_cursor, const cpp_entity*> map_;
    };

    template <CXCursorKind Kind>
    class basic_cpp_entity_ref
    {
    public:
        static CXCursorKind get_kind() STANDARDESE_NOEXCEPT
        {
            return Kind;
        }

        basic_cpp_entity_ref() : cursor_(), name_("")
        {
        }

        basic_cpp_entity_ref(cpp_cursor cur, cpp_name name) : cursor_(cur), name_(std::move(name))
        {
            auto kind           = clang_getCursorKind(cur);
            auto valid_ref_kind = Kind == CXCursor_FirstInvalid || Kind == kind;
            assert(clang_isDeclaration(kind) || (clang_isReference(kind) && valid_ref_kind));
            (void)valid_ref_kind;
        }

        cpp_cursor operator*() const STANDARDESE_NOEXCEPT
        {
            if (clang_getCursorKind(cursor_) != CXCursor_OverloadedDeclRef)
                return clang_getCursorReferenced(cursor_);
            auto num = clang_getNumOverloadedDecls(cursor_);
            if (num == 0u)
                return cursor_;
            return clang_getOverloadedDecl(cursor_, 0u);
        }

        cpp_cursor get() const STANDARDESE_NOEXCEPT
        {
            return cursor_;
        }

        const cpp_entity* get(const cpp_entity_registry& e) const STANDARDESE_NOEXCEPT
        {
            return e.try_lookup(**this);
        }

        const cpp_name& get_name() const STANDARDESE_NOEXCEPT
        {
            return name_;
        }

        /// \returns The scope of the referenced entity.
        cpp_name get_scope() const
        {
            return detail::parse_scope(**this);
        }

        cpp_name get_full_name() const
        {
            std::string scope = get_scope().c_str();
            // need name of the cursor alone, without any scopes
            std::string name = detail::parse_name(**this).c_str();

            if (is_inheriting_ctor())
                // we have a CXCursor_TypeRef of the name of the class
                // need to make it the constructor
                name += +"::" + name;

            return scope.empty() ? name : scope + "::" + name;
        }

    private:
        bool is_inheriting_ctor() const
        {
            if (clang_getCursorKind(cursor_) != CXCursor_TypeRef)
                return false;

            // go backwards
            // see if we have <str>::<str>
            std::string a, b;
            auto        before_scope = true;
            for (auto i = name_.length(); i != 0; --i)
            {
                auto c = name_.c_str()[i - 1];
                if (c == ':' && name_.c_str()[i - 2] == ':')
                {
                    --i;
                    if (!before_scope)
                        // multiple scopes
                        break;

                    before_scope = false;
                }
                else if (before_scope)
                    a += c;
                else if (!before_scope)
                    b += c;
            }

            return a == b;
        }

        cpp_cursor cursor_;
        cpp_name   name_;
    };

    template <CXCursorKind Kind>
    bool operator==(const basic_cpp_entity_ref<Kind>& a, const basic_cpp_entity_ref<Kind>& b)
    {
        return *a == *b;
    }

    template <CXCursorKind Kind>
    bool operator!=(const basic_cpp_entity_ref<Kind>& a, const basic_cpp_entity_ref<Kind>& b)
    {
        return !(a == b);
    }

    using cpp_entity_ref = basic_cpp_entity_ref<CXCursor_FirstInvalid>;
} // namespace standardese

#endif //STANDARDESE_CPP_ENTITY_REGISTRY_HPP_INCLUDED
