// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_HPP_INCLUDED
#define STANDARDESE_COMMENT_HPP_INCLUDED

#include <map>
#include <mutex>

#include <standardese/md_entity.hpp>
#include <standardese/md_blocks.hpp>

namespace standardese
{
    class doc_entity;

    class md_comment final : public md_container
    {
    public:
        static md_entity::type get_entity_type() STANDARDESE_NOEXCEPT
        {
            return md_entity::comment_t;
        }

        static md_ptr<md_comment> make();

        md_entity& add_entity(md_entity_ptr ptr) override;

        md_paragraph& get_brief() STANDARDESE_NOEXCEPT
        {
            assert(begin()->get_entity_type() == md_entity::paragraph_t);
            auto& brief = static_cast<md_paragraph&>(*begin());
            assert(brief.get_section_type() == section_type::brief);
            return brief;
        }

        const md_paragraph& get_brief() const STANDARDESE_NOEXCEPT
        {
            assert(begin()->get_entity_type() == md_entity::paragraph_t);
            auto& brief = static_cast<const md_paragraph&>(*begin());
            assert(brief.get_section_type() == section_type::brief);
            return brief;
        }

        md_ptr<md_comment> clone() const
        {
            auto entity = do_clone(nullptr);
            return md_ptr<md_comment>(static_cast<md_comment*>(entity.release()));
        }

        md_ptr<md_comment> clone(const md_entity& parent) const
        {
            auto entity = do_clone(&parent);
            return md_ptr<md_comment>(static_cast<md_comment*>(entity.release()));
        }

        void set_entity(const doc_entity& entity) const STANDARDESE_NOEXCEPT
        {
            entity_ = &entity;
        }

        bool has_entity() const STANDARDESE_NOEXCEPT
        {
            return entity_ != nullptr;
        }

        const doc_entity& get_entity() const STANDARDESE_NOEXCEPT
        {
            return *entity_;
        }

    protected:
        md_entity_ptr do_clone(const md_entity* parent) const override;

    private:
        md_comment();

        mutable const doc_entity* entity_;

        friend detail::md_ptr_access;
    };

    class comment_id;

    namespace detail
    {
        struct comment_compare
        {
            bool operator()(const comment_id& id_a,
                            const comment_id& id_b) const STANDARDESE_NOEXCEPT;
        };
    } // namespace detail

    /// The identifier of a comment.
    /// Used to specify the entity it refers to.
    class comment_id
    {
    public:
        comment_id(const string& file_name, unsigned line)
        : file_name_or_name_(get_file_name(file_name.c_str())), line_(line)
        {
            assert(line != 0u);
        }

        comment_id(const string& file_name, unsigned line, const string& entity_name)
        : file_name_or_name_('$' + get_file_name(file_name.c_str()) + '$' + entity_name.c_str()),
          line_(line)
        {
            assert(line != 0u);
        }

        explicit comment_id(string name) : file_name_or_name_(std::move(name)), line_(0u)
        {
        }

        bool is_name() const STANDARDESE_NOEXCEPT
        {
            return line_ == 0u;
        }

        bool is_location() const STANDARDESE_NOEXCEPT
        {
            return !is_name() && file_name_or_name_.c_str()[0] != '$';
        }

        bool is_inline_location() const STANDARDESE_NOEXCEPT
        {
            return !is_name() && !is_location();
        }

        string file_name() const STANDARDESE_NOEXCEPT
        {
            assert(!is_name());
            if (is_location())
                return file_name_or_name_;

            assert(is_inline_location());
            std::string result;
            for (auto ptr = file_name_or_name_.c_str() + 1; *ptr != '$'; ++ptr)
                result += *ptr;

            return result;
        }

        unsigned line() const STANDARDESE_NOEXCEPT
        {
            assert(is_location() || is_inline_location());
            return line_;
        }

        string inline_entity_name() const STANDARDESE_NOEXCEPT
        {
            assert(is_inline_location());
            auto ptr = file_name_or_name_.c_str() + 1;
            while (*ptr != '$')
                ++ptr;
            ++ptr;
            return ptr;
        }

        const string& unique_name() const STANDARDESE_NOEXCEPT
        {
            assert(is_name());
            return file_name_or_name_;
        }

    private:
        std::string get_file_name(const std::string& path)
        {
            return path.substr(path.find_last_of("/\\:") + 1);
        }

        string   file_name_or_name_;
        unsigned line_;

        friend detail::comment_compare;
    };

    class comment
    {
    public:
        comment() : content_(md_comment::make()), group_id_(0u), excluded_(false)
        {
            assert(content_);
        }

        bool empty() const STANDARDESE_NOEXCEPT;

        const md_comment& get_content() const STANDARDESE_NOEXCEPT
        {
            return *content_;
        }

        md_comment& get_content() STANDARDESE_NOEXCEPT
        {
            return *content_;
        }

        void set_content(md_ptr<md_comment> content) STANDARDESE_NOEXCEPT
        {
            content_ = std::move(content);
        }

        bool has_unique_name_override() const STANDARDESE_NOEXCEPT
        {
            return !get_unique_name_override().empty();
        }

        const std::string& get_unique_name_override() const STANDARDESE_NOEXCEPT
        {
            return unique_name_override_;
        }

        void set_unique_name_override(std::string name)
        {
            unique_name_override_ = std::move(name);
        }

        bool has_synopsis_override() const STANDARDESE_NOEXCEPT
        {
            return !synopsis_override_.empty();
        }

        const std::string& get_synopsis_override() const STANDARDESE_NOEXCEPT
        {
            return synopsis_override_;
        }

        void set_synopsis_override(const std::string& synopsis, unsigned tab_width);

        bool in_module() const STANDARDESE_NOEXCEPT
        {
            return !module_.empty();
        }

        const std::string& get_module() const STANDARDESE_NOEXCEPT
        {
            return module_;
        }

        void set_module(std::string module)
        {
            module_ = std::move(module);
        }

        bool in_member_group() const STANDARDESE_NOEXCEPT
        {
            return group_id_ != 0u;
        }

        std::size_t member_group_id() const STANDARDESE_NOEXCEPT
        {
            return group_id_;
        }

        void add_to_member_group(std::size_t group_id)
        {
            group_id_ = group_id;
        }

        bool has_group_name() const STANDARDESE_NOEXCEPT
        {
            return !group_name_.empty();
        }

        const std::string& get_group_name() const STANDARDESE_NOEXCEPT
        {
            return group_name_;
        }

        void set_group_name(std::string name)
        {
            group_name_ = std::move(name);
        }

        bool is_excluded() const STANDARDESE_NOEXCEPT
        {
            return excluded_;
        }

        void set_excluded(bool b) STANDARDESE_NOEXCEPT
        {
            excluded_ = b;
        }

    private:
        std::string        unique_name_override_;
        std::string        synopsis_override_;
        std::string        module_;
        std::string        group_name_;
        md_ptr<md_comment> content_;
        std::size_t        group_id_;
        bool               excluded_;
    };

    class cpp_entity;
    class cpp_entity_registry;
    class doc_entity;

    namespace detail
    {
        string get_unique_name(const doc_entity* parent, const string& unique_name,
                               const comment* c);
    }

    class comment_registry
    {
    public:
        bool register_comment(comment_id id, comment c) const;

        const comment* lookup_comment(const cpp_entity& e, const doc_entity* parent) const;

        const comment* lookup_comment(const std::string& module) const;

    private:
        mutable std::mutex mutex_;
        mutable std::map<comment_id, comment, detail::comment_compare> comments_;
    };

    class parser;

    void parse_comments(const parser& p, const char* file_name, const std::string& source);
} // namespace standardese

#endif // STANDARDESE_COMMENT_HPP_INCLUDED
