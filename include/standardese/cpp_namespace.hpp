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
    : public cpp_entity, public cpp_entity_container<cpp_entity>
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

        cpp_namespace(const cpp_name &scope, cpp_name name, cpp_raw_comment comment)
        : cpp_entity(namespace_t, scope, std::move(name), std::move(comment)),
          inline_(false) {}

        void add_entity(cpp_entity_ptr ptr)
        {
            cpp_entity_container::add_entity(std::move(ptr));
        }

        bool is_inline() const STANDARDESE_NOEXCEPT
        {
            return inline_;
        }

    private:
        bool inline_;
    };

    class cpp_namespace_alias
    : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_namespace_alias> parse(cpp_name scope, cpp_cursor cur);

        cpp_namespace_alias(cpp_name scope, cpp_name name, cpp_raw_comment comment, cpp_name target)
        : cpp_entity(namespace_alias_t, std::move(scope), std::move(name), std::move(comment)),
          target_(std::move(target)), unique_(target) {}

        const cpp_name& get_target() const STANDARDESE_NOEXCEPT
        {
            return target_;
        }

        cpp_name get_full_target() const
        {
            return unique_;
        }

    private:
        cpp_name target_, unique_;
    };

    class cpp_using_directive
    : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_using_directive> parse(cpp_cursor cur);

        cpp_using_directive(cpp_name target_scope, cpp_name target_name, cpp_raw_comment comment)
        : cpp_entity(using_directive_t, std::move(target_scope),
                     std::move(target_name), std::move(comment)) {}
    };

    class cpp_using_declaration
    : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_using_declaration> parse(cpp_cursor cur);

        cpp_using_declaration(cpp_name target_scope, cpp_name target_name, cpp_raw_comment comment)
        : cpp_entity(using_declaration_t, std::move(target_scope),
                     std::move(target_name), std::move(comment)) {}
    };
} // namespace standardese

#endif // STANDARDESE_CPP_NAMESPACE_HPP_INCLUDED
