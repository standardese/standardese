// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_SCOPE_STACK_HPP_INCLUDED
#define STANDARDESE_DETAIL_SCOPE_STACK_HPP_INCLUDED

#include <vector>
#include <utility>

#include <standardese/cpp_entity.hpp>

namespace standardese
{
    class cpp_file;
} // namespace standardese

namespace standardese
{
    namespace detail
    {
        class scope_stack
        {
        public:
            // gives the file, i.e. the global scope
            scope_stack(cpp_file* file);

            // adds an entity to the current scope
            // returns true if a new scope was pushed
            bool add_entity(cpp_entity_ptr ptr, cpp_cursor parent);

            // pops the current scope if needed
            // returns true if popped
            bool pop_if_needed(cpp_cursor parent);

            // returns the current parent entity
            const cpp_entity& cur_parent() const STANDARDESE_NOEXCEPT
            {
                return *stack_.back().first;
            }

        private:
            std::vector<std::pair<cpp_entity*, cpp_cursor>> stack_;
        };
    }
} // namespace standardese::detail

#endif // STANDARDESE_DETAIL_SCOPE_STACK_HPP_INCLUDED
