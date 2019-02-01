// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_MARKUP_VISITOR_HPP_INCLUDED
#define STANDARDESE_MARKUP_VISITOR_HPP_INCLUDED

namespace standardese
{
namespace markup
{
    class entity;

    namespace detail
    {
        using visitor_callback_t = void (*)(void* mem, const entity&);

        void call_visit(const entity& e, visitor_callback_t cb, void* mem);

        template <typename Func>
        void visitor_callback(void* mem, const entity& e)
        {
            auto& func = *static_cast<Func*>(mem);
            func(e);
            call_visit(e, &visitor_callback<Func>, mem);
        }
    } // namespace detail

    /// Visits an entity.
    /// \effects Invokes the function passing it the current entity, followed by all its children,
    /// recursively.
    template <typename Func>
    void visit(const entity& e, Func f)
    {
        detail::visitor_callback<Func>(&f, e);
    }
} // namespace markup
} // namespace standardese

#endif // STANDARDESE_MARKUP_VISITOR_HPP_INCLUDED
