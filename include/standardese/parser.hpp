// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_PARSER_HPP_INCLUDED
#define STANDARDESE_PARSER_HPP_INCLUDED

#include <clang-c/Index.h>
#include <memory>
#include <string>
#include <vector>

#include <spdlog/logger.h>

#include <standardese/detail/wrapper.hpp>
#include <standardese/config.hpp>
#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_entity_registry.hpp>

#if CINDEX_VERSION_MAJOR != 0
    #error "require libclang version 0.x"
#endif

#if CINDEX_VERSION_MINOR < 30
    #error "require at least libclang version 0.30 (bundled with 3.7.1)"
#endif

namespace standardese
{
    class translation_unit;

    namespace detail
    {
        struct file_container_impl : cpp_entity_container<cpp_entity>
        {
            void add_file(cpp_entity_ptr e) STANDARDESE_NOEXCEPT
            {
                add_entity(std::move(e));
            }
        };

        class file_container
        {
        public:
            void add_file(cpp_entity_ptr e) const STANDARDESE_NOEXCEPT
            {
                std::unique_lock<std::mutex> lock(mutex_);
                impl_.add_file(std::move(e));
            }

        private:
            mutable std::mutex mutex_;
            mutable file_container_impl impl_;
        };
    } // namespace detail

    /// Parser class used for parsing the C++ classes.
    /// The parser object must live as long as all the translation units.
    class parser
    {
    public:
        parser();

        explicit parser(std::shared_ptr<spdlog::logger> logger);

        parser(parser&&) = delete;
        parser(const parser&) = delete;

        ~parser() STANDARDESE_NOEXCEPT;

        parser& operator=(parser&&) = delete;
        parser& operator=(const parser&) = delete;

        /// Parses a translation unit.
        translation_unit parse(const char *path, const compile_config &c) const;

        const cpp_entity_registry& get_registry() const STANDARDESE_NOEXCEPT
        {
            return registry_;
        }

        const std::shared_ptr<spdlog::logger>& get_logger() const STANDARDESE_NOEXCEPT
        {
            return logger_;
        }

        comment_config& get_comment_config() STANDARDESE_NOEXCEPT
        {
            return comment_;
        }

        const comment_config& get_comment_config() const STANDARDESE_NOEXCEPT
        {
            return comment_;
        }

        output_config& get_output_config() STANDARDESE_NOEXCEPT
        {
            return output_;
        }

        const output_config& get_output_config() const STANDARDESE_NOEXCEPT
        {
            return output_;
        }

    private:
        struct deleter
        {
            void operator()(CXIndex idx) const STANDARDESE_NOEXCEPT;
        };

        cpp_entity_registry registry_;
        comment_config comment_;
        output_config output_;
        detail::wrapper<CXIndex, deleter> index_;
        std::shared_ptr<spdlog::logger> logger_;
        detail::file_container files_;
    };
} // namespace standardese

#endif // STANDARDESE_PARSER_HPP_INCLUDED
