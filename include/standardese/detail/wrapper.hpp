// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_WRAPPER_HPP_INCLUDED
#define STANDARDESE_DETAIL_WRAPPER_HPP_INCLUDED

#include <cassert>
#include <type_traits>
#include <utility>

#include <standardese/noexcept.hpp>

namespace standardese
{
    namespace detail
    {
        // validates libclang object
        // this is just an extra layer of security
        template <typename T>
        void validate(T* obj)
        {
            assert(obj);
        }

        template <typename T, class Deleter>
        class wrapper : Deleter
        {
            static_assert(std::is_pointer<T>::value, "");

        public:
            wrapper(T obj) STANDARDESE_NOEXCEPT : obj_(obj)
            {
                validate(obj_);
            }

            wrapper(wrapper&& other) STANDARDESE_NOEXCEPT : obj_(other.obj_)
            {
                other.obj_ = nullptr;
            }

            ~wrapper() STANDARDESE_NOEXCEPT
            {
                if (obj_)
                    static_cast<Deleter&> (*this)(obj_);
            }

            wrapper& operator=(wrapper&& other) STANDARDESE_NOEXCEPT
            {
                wrapper tmp(std::move(other));
                swap(*this, tmp);
                return *this;
            }

            friend void swap(wrapper& a, wrapper& b) STANDARDESE_NOEXCEPT
            {
                std::swap(a.obj_, b.obj_);
            }

            T get() const STANDARDESE_NOEXCEPT
            {
                validate(obj_);
                return obj_;
            }

        private:
            T obj_;
        };
    }
} // namespace standardese::detail

#endif // STANDARDESE_DETAIL_WRAPPER_HPP_INCLUDED
