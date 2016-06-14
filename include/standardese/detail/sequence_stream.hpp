// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_SEQUENCE_STREAM_HPP_INCLUDED
#define STANDARDESE_DETAIL_SEQUENCE_STREAM_HPP_INCLUDED

#include <cstddef>
#include <iterator>

#include <standardese/noexcept.hpp>

namespace standardese
{
    namespace detail
    {
        // Exposes a sequence with a streaming interface.
        // It also allows out-of-range access.
        template <typename Iter>
        class sequence_stream
        {
        public:
            using iterator   = Iter;
            using value_type = typename std::iterator_traits<Iter>::value_type;

            template <typename Cont>
            explicit sequence_stream(Cont& array, value_type out_of_range = {})
            : out_of_range_(std::move(out_of_range))
            {
                using std::begin;
                using std::end;

                begin_ = begin(array);
                cur_   = begin(array);
                end_   = end(array);
            }

            const value_type& peek() const STANDARDESE_NOEXCEPT
            {
                if (cur_ == end_)
                    return out_of_range_;
                return *cur_;
            }

            value_type peek(std::size_t offset) const STANDARDESE_NOEXCEPT
            {
                if (std::ptrdiff_t(offset) > left())
                    return out_of_range_;
                return *std::next(cur_, offset);
            }

            void bump() STANDARDESE_NOEXCEPT
            {
                if (cur_ != end_)
                    ++cur_;
            }

            value_type get() STANDARDESE_NOEXCEPT
            {
                auto result = peek();
                bump();
                return result;
            }

            std::size_t size() const STANDARDESE_NOEXCEPT
            {
                return std::size_t(std::distance(begin_, end_));
            }

            std::ptrdiff_t left() const STANDARDESE_NOEXCEPT
            {
                return std::distance(cur_, end_);
            }

            bool done() const STANDARDESE_NOEXCEPT
            {
                return cur_ == end_;
            }

        private:
            Iter       begin_, cur_, end_;
            value_type out_of_range_;
        };
    }
} // namespace standardese::detail

#endif // STANDARDESE_DETAIL_SEQUENCE_STREAM_HPP_INCLUDED
