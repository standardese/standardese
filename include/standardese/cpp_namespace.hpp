// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_NAMESPACE_HPP_INCLUDED
#define STANDARDESE_CPP_NAMESPACE_HPP_INCLUDED

#include <standardese/cpp_entity.hpp>

namespace standardese
{
    class cpp_cursor;
    class parser;

    class cpp_namespace
    : public cpp_entity, public cpp_entity_container
    {
    public:
        class parser : public cpp_entity_parser
        {
        public:
            parser(const cpp_name &scope, cpp_cursor cur);

            void add_entity(cpp_entity_ptr ptr) override
            {
                ns_->add_entity(std::move(ptr));
            }

            cpp_name scope_name() override
            {
                return ns_->get_name();
            }

            cpp_entity_ptr finish(const standardese::parser &par) override;

        private:
            cpp_ptr<cpp_namespace> ns_;
        };

        cpp_name get_unique_name() const override
        {
            return full_name_;
        }

        bool is_inline() const STANDARDESE_NOEXCEPT
        {
            return inline_;
        }

    private:
        cpp_namespace(const cpp_name &scope, cpp_name name, cpp_comment comment)
        : cpp_entity(std::move(name), std::move(comment)),
          full_name_(scope + get_name()), inline_(false) {}

        cpp_name full_name_;
        bool inline_;

        friend parser;
    };
} // namespace standardese

#endif // STANDARDESE_CPP_NAMESPACE_HPP_INCLUDED
