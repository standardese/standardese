// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/template_processor.hpp>

#include <algorithm>
#include <cstring>
#include <new>
#include <type_traits>
#include <vector>

#include <standardese/output.hpp>

using namespace standardese;

template_config::template_config() : delimiter_begin_("{{"), delimiter_end_("}}")
{
    set_command(template_command::generate_doc, "doc");
    set_command(template_command::generate_doc_text, "doc_text");
    set_command(template_command::generate_synopsis, "doc_synopsis");

    set_command(template_command::name, "name");
    set_command(template_command::index_name, "index_name");
    set_command(template_command::unique_name, "unique_name");

    set_command(template_command::for_each, "for");
    set_command(template_command::if_clause, "if");
    set_command(template_command::else_if_clause, "else_if");
    set_command(template_command::else_clause, "else");
    set_command(template_command::end, "end");

    set_operation(template_if_operation::name, "name");
    set_operation(template_if_operation::first_child, "first_child");
    set_operation(template_if_operation::has_children, "has_children");
    set_operation(template_if_operation::inline_entity, "inline_entity");
    set_operation(template_if_operation::member_group, "member_group");
}

void template_config::set_command(template_command cmd, std::string str)
{
    commands_[static_cast<std::size_t>(cmd)] = std::move(str);
}

template_command template_config::get_command(const std::string& str) const
{
    auto res = try_get_command(str);
    if (res == template_command::invalid)
        throw std::logic_error(fmt::format("invalid template command '{}'", str));
    return res;
}

template_command template_config::try_get_command(const std::string& str) const STANDARDESE_NOEXCEPT
{
    auto iter = std::find(std::begin(commands_), std::end(commands_), str);
    if (iter == std::end(commands_))
        return template_command::invalid;
    return static_cast<template_command>(iter - std::begin(commands_));
}

void template_config::set_operation(template_if_operation cmd, std::string str)
{
    if_operations_[static_cast<std::size_t>(cmd)] = std::move(str);
}

template_if_operation template_config::get_operation(const std::string& str) const
{
    auto res = try_get_operation(str);
    if (res == template_if_operation::invalid)
        throw std::logic_error(fmt::format("invalid template if operation '{}'", str));
    return res;
}

template_if_operation template_config::try_get_operation(const std::string& str) const
    STANDARDESE_NOEXCEPT
{
    auto iter = std::find(std::begin(if_operations_), std::end(if_operations_), str);
    if (iter == std::end(if_operations_))
        return template_if_operation::invalid;
    return static_cast<template_if_operation>(iter - std::begin(if_operations_));
}

namespace
{
    using standardese::index;

    std::string read_arg(const char*& ptr, const char* end)
    {
        while (ptr != end && std::isspace(*ptr))
            ++ptr;

        std::string result;
        while (ptr != end && !std::isspace(*ptr))
            result += *ptr++;

        return result;
    }

    template <typename Func, typename Func2>
    void parse_commands(spdlog::logger& log, const template_config& config, const char* input,
                        Func handle, Func2 skip)
    {
        auto last_match = input;
        // while we find begin delimiter starting at last_match
        while (auto match = std::strstr(last_match, config.delimiter_begin().c_str()))
        {
            // append characters between matches
            skip(last_match, match);

            // find end delimiter
            auto start = match + config.delimiter_begin().size();
            auto last  = std::strstr(start, config.delimiter_end().c_str());
            if (!last)
                break;
            auto end = last + config.delimiter_end().size();

            auto cur_command   = read_arg(start, last);
            auto prefix        = "standardese_";
            auto prefix_length = std::strlen(prefix);
            if (std::strncmp(cur_command.c_str(), prefix, prefix_length) == 0)
            {
                auto cmd = config.try_get_command(cur_command.c_str() + prefix_length);
                if (cmd != template_command::invalid)
                    // process command
                    handle(cmd, start, last, end);
                else
                    log.warn("unknown template command '{}'", cur_command);
            }
            else
                // ignore command completely
                skip(match, end);

            // set last match after the end location
            last_match = end;
        }

        // append characters between last match and end
        skip(last_match, nullptr);
    }

    class stack
    {
    public:
        stack(const parser& p, const index& idx) STANDARDESE_NOEXCEPT : parser_(&p), idx_(&idx)
        {
        }

        std::string& get_buffer() STANDARDESE_NOEXCEPT
        {
            if (stack_.empty())
                return buffer_;
            else if (auto loop = stack_.back().get_for_loop())
                return loop->buffer;
            else if (auto clause = stack_.back().get_if_clause())
                return clause->buffer;
            else if (auto clause = stack_.back().get_else_clause())
                return clause->buffer;
            assert(false);
            return buffer_;
        }

        const doc_entity* lookup_var(const std::string& name) const STANDARDESE_NOEXCEPT
        {
            for (auto iter = stack_.rbegin(); iter != stack_.rend(); ++iter)
            {
                if (auto loop = iter->get_for_loop())
                {
                    if (loop->cur == loop->end)
                        // inside a loop that is skipped
                        return nullptr;
                    else if (loop->loop_var == name)
                        return &*loop->cur;
                }
            }

            auto entity = idx_->try_lookup(name);
            if (!entity)
                parser_->get_logger()->warn("unable to find entity named '{}'", name);
            return entity;
        }

        void push_loop(const doc_entity& e, std::string loop_var, const char* ptr)
        {
            stack_.emplace_back(for_loop(e, std::move(loop_var), ptr));
        }

        void push_if(bool v)
        {
            stack_.emplace_back(if_clause(v));
        }

        void on_end(const char*& ptr)
        {
            if (stack_.empty())
                parser_->get_logger()->warn("end block without active block");
            else if (auto loop = stack_.back().get_for_loop())
                end_loop(ptr, loop);
            else if (auto clause = stack_.back().get_if_clause())
                end_if(clause);
            else if (auto clause = stack_.back().get_else_clause())
                end_else(clause);
            else
                assert(false);
        }

        void on_else(bool else_if)
        {
            if (stack_.empty() || stack_.back().get_type() != if_clause_t)
                parser_->get_logger()->warn("else block without if");
            else if (auto clause = stack_.back().get_if_clause())
            {
                auto value = end_if(clause);
                stack_.emplace_back(else_clause(!value, else_if));
            }
        }

        const parser& get_parser() const STANDARDESE_NOEXCEPT
        {
            return *parser_;
        }

    private:
        struct for_loop
        {
            std::string                          loop_var, buffer;
            doc_entity_container::const_iterator cur;
            doc_entity_container::const_iterator end;
            const char*                          start;

            explicit for_loop(const doc_entity& e, std::string var, const char* start)
            : loop_var(std::move(var)), cur(e.begin()), end(e.end()), start(start)
            {
            }
        };

        void end_loop(const char*& ptr, for_loop* loop)
        {
            if (loop->cur == loop->end)
                // loop is skipped, just ignore
                stack_.pop_back();
            else if (std::next(loop->cur) != loop->end)
            {
                // next iteration
                ++loop->cur;
                ptr = loop->start;
            }
            else
            {
                // finish loop
                auto res = std::move(loop->buffer);
                stack_.pop_back();
                get_buffer() += std::move(res);
            }
        }

        struct if_clause
        {
            std::string buffer;
            bool        value;

            explicit if_clause(bool v) : value(v)
            {
            }
        };

        // returns true if any if was chosen
        bool end_if(if_clause* clause)
        {
            auto value = clause->value;

            if (clause->value)
            {
                // take if clause
                auto res = std::move(clause->buffer);
                stack_.pop_back();
                get_buffer() += std::move(res);
            }
            else
            {
                // skip if clause
                stack_.pop_back();
            }

            if (!stack_.empty() && stack_.back().get_type() == else_clause_t)
            {
                auto clause = stack_.back().get_else_clause();
                if (clause->owns_if)
                {
                    // pop else clause as well
                    value = value || !clause->value;
                    end_else(clause);
                }
            }

            return value;
        }

        struct else_clause
        {
            std::string buffer;
            bool        value;
            bool        owns_if; // true if it shares an end with if

            explicit else_clause(bool v, bool owns) : value(v), owns_if(owns)
            {
            }
        };

        void end_else(else_clause* clause)
        {
            if (clause->value)
            {
                // take else
                auto res = std::move(clause->buffer);
                stack_.pop_back();
                get_buffer() += std::move(res);
            }
            else
            {
                // skip else
                stack_.pop_back();
            }
        }

        enum state_type
        {
            for_loop_t,
            if_clause_t,
            else_clause_t,
        };

        class state
        {
        public:
            state(for_loop loop) : type_(for_loop_t)
            {
                ::new (as_void()) for_loop(std::move(loop));
            }

            state(if_clause c) : type_(if_clause_t)
            {
                ::new (as_void()) if_clause(std::move(c));
            }

            state(else_clause c) : type_(else_clause_t)
            {
                ::new (as_void()) else_clause(std::move(c));
            }

            state(state&& other) STANDARDESE_NOEXCEPT : type_(other.type_)
            {
                switch (other.type_)
                {
                case for_loop_t:
                    ::new (as_void()) for_loop(std::move(*other.get_for_loop()));
                    break;
                case if_clause_t:
                    ::new (as_void()) if_clause(std::move(*other.get_if_clause()));
                    break;
                case else_clause_t:
                    ::new (as_void()) else_clause(std::move(*other.get_else_clause()));
                    break;
                }
            }

            ~state() STANDARDESE_NOEXCEPT
            {
                switch (type_)
                {
                case for_loop_t:
                    get_for_loop()->~for_loop();
                    break;
                case if_clause_t:
                    get_if_clause()->~if_clause();
                    break;
                case else_clause_t:
                    get_else_clause()->~else_clause();
                    break;
                }
            }

            state& operator=(state&&) STANDARDESE_NOEXCEPT = delete;

            state_type get_type() const STANDARDESE_NOEXCEPT
            {
                return type_;
            }

            for_loop* get_for_loop() STANDARDESE_NOEXCEPT
            {
                return type_ == for_loop_t ? static_cast<for_loop*>(as_void()) : nullptr;
            }

            const for_loop* get_for_loop() const STANDARDESE_NOEXCEPT
            {
                return type_ == for_loop_t ? static_cast<const for_loop*>(as_void()) : nullptr;
            }

            if_clause* get_if_clause() STANDARDESE_NOEXCEPT
            {
                return type_ == if_clause_t ? static_cast<if_clause*>(as_void()) : nullptr;
            }

            const if_clause* get_if_clause() const STANDARDESE_NOEXCEPT
            {
                return type_ == if_clause_t ? static_cast<const if_clause*>(as_void()) : nullptr;
            }

            else_clause* get_else_clause() STANDARDESE_NOEXCEPT
            {
                return type_ == else_clause_t ? static_cast<else_clause*>(as_void()) : nullptr;
            }

            const else_clause* get_else_clause() const STANDARDESE_NOEXCEPT
            {
                return type_ == else_clause_t ? static_cast<const else_clause*>(as_void()) :
                                                nullptr;
            }

        private:
            void* as_void() STANDARDESE_NOEXCEPT
            {
                return &storage_;
            }

            const void* as_void() const STANDARDESE_NOEXCEPT
            {
                return &storage_;
            }

            using storage = typename std::aligned_union<1, for_loop>::type;
            storage    storage_;
            state_type type_;
        };

        std::vector<state> stack_;
        std::string        buffer_;
        const parser*      parser_;
        const index*       idx_;
    };

    md_ptr<md_document> get_documentation(const stack& vars, const index& i,
                                          const std::string& name)
    {
        auto entity = vars.lookup_var(name);
        if (!entity)
            return nullptr;

        auto doc = md_document::make("");
        entity->generate_documentation(vars.get_parser(), i, *doc);
        return doc;
    }

    md_ptr<md_document> get_synopsis(const stack& vars, const std::string& name)
    {
        auto entity = vars.lookup_var(name);
        if (!entity)
            return nullptr;

        auto              doc = md_document::make("");
        code_block_writer writer(*doc);
        entity->generate_synopsis(vars.get_parser(), writer);
        doc->add_entity(writer.get_code_block());

        return doc;
    }

    md_ptr<md_document> get_documentation_text(const stack& vars, const std::string& name)
    {
        auto entity = vars.lookup_var(name);
        if (!entity)
            return nullptr;

        auto doc = md_document::make("");
        doc->add_entity(entity->get_comment().get_content().clone(*doc));
        return doc;
    }

    std::string write_document(const parser& p, md_ptr<md_document> doc,
                               const std::string& format_name)
    {
        auto format = make_output_format(format_name);
        if (!format)
        {
            p.get_logger()->warn("invalid format name '{}'", format_name);
            return "";
        }

        string_output output;
        format->render(output, *doc);

        return output.get_string();
    }

    bool get_if_value(const stack& s, const template_config& config, const doc_entity* entity,
                      const char*& ptr, const char* last)
    {
        auto op = read_arg(ptr, last);
        switch (config.get_operation(op))
        {
        case template_if_operation::name:
        {
            auto name = read_arg(ptr, last);
            return entity->get_unique_name() == name.c_str();
        }
        case template_if_operation::first_child:
        {
            auto name  = read_arg(ptr, last);
            auto other = s.lookup_var(name);
            if (!other || other->begin() == other->end())
                return false;
            return entity == &*other->begin();
        }
        case template_if_operation::has_children:
            return entity->begin() != entity->end();
        case template_if_operation::inline_entity:
            return s.get_parser().get_output_config().is_set(output_flag::inline_documentation)
                   && is_inline_cpp_entity(entity->get_cpp_entity_type());
        case template_if_operation::member_group:
            return entity->get_entity_type() == doc_entity::member_group_t;
        case template_if_operation::invalid:
            s.get_parser().get_logger()->warn("unknown if operation '{}'", op);
            break;
        }

        return false;
    }
}

std::string standardese::process_template(const parser& p, const index& i,
                                          const template_config& config, const std::string& input)
{
    stack s(p, i);
    auto handle = [&](template_command cur_command, const char* ptr, const char* last,
                      const char*& end) {
        switch (cur_command)
        {
        case template_command::generate_doc:
            if (auto doc = get_documentation(s, i, read_arg(ptr, last)))
                s.get_buffer() += write_document(p, std::move(doc), read_arg(ptr, last));
            break;
        case template_command::generate_synopsis:
            if (auto doc = get_synopsis(s, read_arg(ptr, last)))
                s.get_buffer() += write_document(p, std::move(doc), read_arg(ptr, last));
            break;
        case template_command::generate_doc_text:
            if (auto doc = get_documentation_text(s, read_arg(ptr, last)))
                s.get_buffer() += write_document(p, std::move(doc), read_arg(ptr, last));
            break;

        case template_command::name:
        {
            auto entity = s.lookup_var(read_arg(ptr, last));
            if (entity)
                s.get_buffer() += entity->get_name().c_str();
            break;
        }
        case template_command::unique_name:
        {
            auto entity = s.lookup_var(read_arg(ptr, last));
            if (entity)
                s.get_buffer() += entity->get_unique_name().c_str();
            break;
        }
        case template_command::index_name:
        {
            auto entity = s.lookup_var(read_arg(ptr, last));
            if (entity)
                s.get_buffer() += entity->get_index_name(true).c_str();
            break;
        }

        case template_command::for_each:
        {
            auto var_name = read_arg(ptr, last);
            auto entity   = s.lookup_var(read_arg(ptr, last));
            if (!entity)
                break;
            s.push_loop(*entity, var_name, end);
            break;
        }
        case template_command::else_if_clause:
            // add an else_if else, then regular if
            s.on_else(true);
        case template_command::if_clause:
        {
            auto entity = s.lookup_var(read_arg(ptr, last));
            if (!entity)
                break;
            s.push_if(get_if_value(s, config, entity, ptr, last));
            break;
        }
        case template_command::else_clause:
            s.on_else(false);
            break;
        case template_command::end:
            s.on_end(end);
            break;

        case template_command::invalid:
            assert(false);
        }
    };
    parse_commands(*p.get_logger(), config, input.c_str(), handle,
                   [&](const char* begin, const char* end) {
                       if (!end)
                           s.get_buffer() += begin;
                       else
                           s.get_buffer().append(begin, end - begin);
                   });
    return s.get_buffer();
}
