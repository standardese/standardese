// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DOC_ENTITY_HPP_INCLUDED
#define STANDARDESE_DOC_ENTITY_HPP_INCLUDED

#include <cassert>
#include <unordered_set>

#include <cppast/code_generator.hpp>
#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_namespace.hpp>

#include "index.hpp"
#include <standardese/comment/doc_comment.hpp>
#include <standardese/markup/code_block.hpp>
#include <standardese/markup/documentation.hpp>
#include <standardese/markup/index.hpp>

namespace standardese
{
/// The configuration of the synopsis.
class synopsis_config
{
public:
    enum flag
    {
        show_complex_noexcept,     //< If set, complex noexcept conditions will be shown.
        show_macro_replacement,    //< If set, replacement text of a macro will be shown.
        show_group_output_section, //< If set, the name of the group will be shown as output
                                   // section.
        separate_members,          //< If set, a newline is added between
                                   // members of a class or struct.
        _flag_set_size,            //< \exclude
    };

    using flags = type_safe::flag_set<flag>;

    /// \returns The default hidden name.
    static const char* default_hidden_name() noexcept;

    /// \returns The default tab width.
    static unsigned default_tab_width() noexcept;

    /// \returns The default flags.
    static flags default_flags() noexcept;

    /// \effects Creates the default synopsis config.
    synopsis_config()
    : hidden_name_(default_hidden_name()), tab_width_(default_tab_width()), flags_(default_flags())
    {}

    /// \returns The name being output when an identifier is being excluded.
    const std::string& hidden_name() const noexcept
    {
        return hidden_name_;
    }

    /// \effects Sets the hidden name.
    void set_hidden_name(std::string str)
    {
        hidden_name_ = std::move(str);
    }

    /// \returns The number of space characters in a tab.
    unsigned tab_width() const noexcept
    {
        return tab_width_;
    }

    /// \effects Sets the tab width.
    void set_tab_width(unsigned width) noexcept
    {
        tab_width_ = width;
    }

    /// \returns Whether or not the given flag is set.
    bool is_flag_set(flag f) const noexcept
    {
        return flags_.is_set(f);
    }

    /// \effects Sets or resets the given flag.
    void set_flag(flag f, bool val) noexcept
    {
        flags_.set(f, val);
    }

private:
    std::string hidden_name_;
    unsigned    tab_width_;
    flags       flags_;
};

/// The configuration of the generated documentation.
class generation_config
{
public:
    enum flag
    {
        document_uncommented, //< Generate documentation for uncommented entities,
        /// even if they have no documented child entities.

        inline_doc, //< Show documentation of entities like parameters inline in the parent
                    // documentation.

        _flag_set_size, //< \exclude
    };

    using flags = type_safe::flag_set<flag>;

    /// \returns The default flags.
    static flags default_flags() noexcept;

    /// \returns The default index order.
    static entity_index::order default_order() noexcept;

    /// \effects Creates the default configuration.
    generation_config() : flags_(default_flags()), order_(default_order()) {}

    /// \returns Whether or not the given flag is set.
    bool is_flag_set(flag f) const noexcept
    {
        return flags_.is_set(f);
    }

    /// \effects Sets the given flag to the given value.
    void set_flag(flag f, bool v) noexcept
    {
        flags_.set(f, v);
    }

    /// \returns The entity index order.
    /// \notes This must be manually passed to the generate function!
    entity_index::order order() const noexcept
    {
        return order_;
    }

    /// \effects Sets the entity index order.
    void set_order(entity_index::order order) noexcept
    {
        order_ = order;
    }

private:
    flags               flags_;
    entity_index::order order_;
};

namespace detail
{
    struct inline_entity_list
    {
        markup::unordered_list::builder params;
        markup::unordered_list::builder tparams;
        markup::unordered_list::builder bases;
        markup::unordered_list::builder enumerators;
        markup::unordered_list::builder members;

        explicit inline_entity_list(const std::string& link_name)
        : params(markup::block_id(link_name + "-params")),
          tparams(markup::block_id(link_name + "-tparams")),
          bases(markup::block_id(link_name + "-bases")),
          enumerators(markup::block_id(link_name + "-enumerators")),
          members(markup::block_id(link_name + "-members"))
        {}
    };

    class markdown_code_generator;
} // namespace detail

/// A documentation entity.
///
/// It combines the [cppast::cpp_entity]() with the [comment::doc_comment]().
/// User data of the [cppast::cpp_entity]() will point to the corresponding doc entity.
class doc_entity
{
public:
    doc_entity(const doc_entity&) = delete;
    doc_entity& operator=(const doc_entity&) = delete;

    virtual ~doc_entity() = default;

    /// \returns Whether or not that entity is excluded.
    /// \notes If that is the case, the dynamic type will be [standardese::doc_excluded_entity]().
    bool is_excluded() const noexcept;

    enum entity_kind
    {
        excluded,      //< [standardese::doc_excluded_entity]()
        member_group,  //< [standardese::doc_member_group_entity]()
        cpp_entity,    //< [standardese::doc_cpp_entity]()
        metadata,      //< [standardese::doc_metadata_entity]()
        cpp_namespace, //< [standardese::doc_cpp_namespace]()
        cpp_file,      //< [standardese::doc_cpp_file]()
    };

    /// \returns The kind of entity.
    entity_kind kind() const noexcept
    {
        return do_get_kind();
    }

    /// \returns The link name of the entity.
    const std::string& link_name() const noexcept
    {
        return link_name_;
    }

    /// \returns The id of the block where the entity is documented.
    markup::block_id get_documentation_id() const
    {
        return do_get_id();
    }

    /// \returns The comment of the entity.
    type_safe::optional_ref<const comment::doc_comment> comment() const noexcept
    {
        return comment_;
    }

    using iterator = markup::detail::vector_ptr_iterator<doc_entity>;

    /// \returns An iterator to the first child.
    iterator begin() const noexcept
    {
        return children_.begin();
    }

    /// \returns An iterator one past the last child.
    iterator end() const noexcept
    {
        return children_.end();
    }

    /// \returns The parent of the entity.
    type_safe::optional_ref<const doc_entity> parent() const noexcept
    {
        return parent_;
    }

    /// \returns Whether or not it was injected and is not actually a child of the parent.
    bool is_injected() const noexcept
    {
        return injected_;
    }

    /// \effects Marks an entity as injected.
    void mark_injected() noexcept
    {
        injected_ = true;
    }

private:
    doc_entity(std::string link_name, type_safe::optional_ref<const comment::doc_comment> comment)
    : link_name_(std::move(link_name)), comment_(comment)
    {}

    template <typename T>
    class basic_builder
    {
    public:
        void add_child(std::unique_ptr<doc_entity> child)
        {
            child->parent_ = type_safe::ref(peek());
            result_->children_.push_back(std::move(child));
        }

        std::unique_ptr<T> finish()
        {
            return std::move(result_);
        }

    protected:
        explicit basic_builder(std::unique_ptr<T> result) : result_(std::move(result)) {}

        std::size_t size() const noexcept
        {
            return result_->children_.size();
        }

        T& peek() noexcept
        {
            return *result_;
        }

        void set_comment(type_safe::optional_ref<const comment::doc_comment> comment)
        {
            result_->comment_ = comment;
        }

    private:
        std::unique_ptr<T> result_;
    };

    /// \exclude
    virtual entity_kind do_get_kind() const noexcept = 0;

    /// \exclude
    virtual markup::block_id do_get_id() const = 0;

    /// \exclude
    virtual std::unique_ptr<markup::documentation_entity> do_generate_documentation(
        const generation_config& gen_config, const synopsis_config& syn_config,
        const cppast::cpp_entity_index&                     index,
        type_safe::optional_ref<detail::inline_entity_list> inlines,
        std::unique_ptr<markup::code_block>                 synopsis) const = 0;

    /// \exclude
    virtual cppast::code_generator::generation_options do_get_generation_options(
        const synopsis_config& config, bool is_main) const = 0;

    /// \exclude
    virtual void do_generate_synopsis_prefix(const cppast::code_generator::output& output,
                                             const synopsis_config& config, bool is_main, type_safe::flag& needs_newline) const
    {
        (void)output;
        (void)config;
        (void)is_main;
    }

    /// \exclude
    virtual void do_generate_code(cppast::code_generator& generator) const = 0;

    std::string                                         link_name_;
    std::vector<std::unique_ptr<doc_entity>>            children_;
    type_safe::optional_ref<const doc_entity>           parent_;
    type_safe::optional_ref<const comment::doc_comment> comment_;
    bool                                                injected_ = false;

    friend class detail::markdown_code_generator;
    friend std::unique_ptr<markup::code_block> generate_synopsis(
        const synopsis_config& config, const cppast::cpp_entity_index& index,
        const doc_entity& entity);

    friend std::unique_ptr<markup::documentation_entity> generate_documentation(
        const generation_config& gen_config, const synopsis_config& syn_config,
        const cppast::cpp_entity_index& index, const doc_entity& entity);

    friend class doc_excluded_entity;
    friend class doc_cpp_entity;
    friend class doc_metadata_entity;
    friend class doc_member_group_entity;
    friend class doc_cpp_namespace;
    friend class doc_cpp_file;
};

/// Generates synopsis for that entity.
/// \returns The synopsis of that entity.
std::unique_ptr<markup::code_block> generate_synopsis(const synopsis_config&          config,
                                                      const cppast::cpp_entity_index& index,
                                                      const doc_entity&               entity);

/// Generates documentation for that entity.
/// \returns The documentation of that entity.
std::unique_ptr<markup::documentation_entity> generate_documentation(
    const generation_config& gen_config, const synopsis_config& syn_config,
    const cppast::cpp_entity_index& index, const doc_entity& entity);

/// Documentation entity that is being marked as excluded.
///
/// This will be the user data of all excluded [cppast::cpp_entity]().
class doc_excluded_entity final : public doc_entity
{
public:
    doc_excluded_entity() : doc_entity("<excluded>", nullptr) {}

private:
    entity_kind do_get_kind() const noexcept override
    {
        return excluded;
    }

    markup::block_id do_get_id() const override
    {
        return markup::block_id("");
    }

    std::unique_ptr<markup::documentation_entity> do_generate_documentation(
        const generation_config&, const synopsis_config&, const cppast::cpp_entity_index&,
        type_safe::optional_ref<detail::inline_entity_list>,
        std::unique_ptr<markup::code_block>) const override
    {
        return nullptr;
    }

    cppast::code_generator::generation_options do_get_generation_options(const synopsis_config&,
                                                                         bool) const override
    {
        return {};
    }

    void do_generate_code(cppast::code_generator&) const override {}
};

/// A regular documentation entity.
///
/// This will be the user data of all non-excluded [cppast::cpp_entity]() that are neither files nor
/// namespaces.
class doc_cpp_entity final : public doc_entity
{
public:
    /// Builds an entity.
    class builder : public doc_entity::basic_builder<doc_cpp_entity>
    {
    public:
        builder(std::string link_name, type_safe::object_ref<const cppast::cpp_entity> entity,
                type_safe::optional_ref<const comment::doc_comment> comment);
    };

    /// \returns The corresponding entity.
    const cppast::cpp_entity& entity() const noexcept
    {
        return *entity_;
    }

    /// \returns Whether or not the entity is in a member group.
    /// If that is the case, the parent will be [standardese::doc_member_group_entity]().
    bool in_member_group() const noexcept
    {
        return group_member_no_.has_value();
    }

    /// \returns Whether or not the entity is the main entity of the group, if it is in a group.
    type_safe::optional<bool> is_group_main() const noexcept
    {
        return group_member_no_.map([](unsigned i) { return i == 1u; });
    }

private:
    doc_cpp_entity(std::string link_name, type_safe::object_ref<const cppast::cpp_entity> entity,
                   type_safe::optional_ref<const comment::doc_comment> comment)
    : doc_entity(std::move(link_name), comment), entity_(entity)
    {}

    entity_kind do_get_kind() const noexcept override
    {
        return cpp_entity;
    }

    markup::block_id do_get_id() const override
    {
        if (in_member_group() || !comment())
            return parent().value().get_documentation_id();
        else
            return markup::block_id(link_name());
    }

    std::unique_ptr<markup::documentation_entity> do_generate_documentation(
        const generation_config& gen_config, const synopsis_config& syn_config,
        const cppast::cpp_entity_index&                     index,
        type_safe::optional_ref<detail::inline_entity_list> inlines,
        std::unique_ptr<markup::code_block>                 synopsis) const override;

    cppast::code_generator::generation_options do_get_generation_options(
        const synopsis_config& config, bool is_main) const override;

    void do_generate_synopsis_prefix(const cppast::code_generator::output& output,
                                     const synopsis_config& config, bool is_main, type_safe::flag& needs_newline) const override;

    void do_generate_code(cppast::code_generator& generator) const override;

    type_safe::optional<unsigned>                   group_member_no_;
    type_safe::object_ref<const cppast::cpp_entity> entity_;

    friend class doc_member_group_entity;
};

/// A metadata only C++ entity.
///
/// This will be the user data of all entities that can't be documented but still have metadata.
/// Examples would be include directives or using declarations.
class doc_metadata_entity final : public doc_entity
{
public:
    /// Builds an entity.
    class builder : public doc_entity::basic_builder<doc_metadata_entity>
    {
    public:
        builder(type_safe::object_ref<const cppast::cpp_entity>   entity,
                type_safe::object_ref<const comment::doc_comment> comment);
    };

    /// \returns The corresponding entity.
    const cppast::cpp_entity& entity() const noexcept
    {
        return *entity_;
    }

private:
    doc_metadata_entity(type_safe::object_ref<const cppast::cpp_entity>   entity,
                        type_safe::object_ref<const comment::doc_comment> comment)
    : doc_entity(entity->name(), comment), entity_(entity)
    {}

    entity_kind do_get_kind() const noexcept override
    {
        return doc_entity::metadata;
    }

    markup::block_id do_get_id() const override
    {
        return parent().value().get_documentation_id();
    }

    std::unique_ptr<markup::documentation_entity> do_generate_documentation(
        const generation_config& gen_config, const synopsis_config& syn_config,
        const cppast::cpp_entity_index&                     index,
        type_safe::optional_ref<detail::inline_entity_list> inlines,
        std::unique_ptr<markup::code_block>                 synopsis) const override;

    cppast::code_generator::generation_options do_get_generation_options(
        const synopsis_config& config, bool is_main) const override;

    void do_generate_synopsis_prefix(const cppast::code_generator::output& output,
                                     const synopsis_config& config, bool is_main, type_safe::flag& needs_newline) const override;

    void do_generate_code(cppast::code_generator& generator) const override;

    type_safe::object_ref<const cppast::cpp_entity> entity_;
};

/// The documentation entity representing a member group.
///
/// The first entity in the group will have it as user data.
class doc_member_group_entity final : public doc_entity
{
public:
    /// Builds a member group.
    class builder : public doc_entity::basic_builder<doc_member_group_entity>
    {
    public:
        builder(std::string link_name)
        : basic_builder(std::unique_ptr<doc_member_group_entity>(
              new doc_member_group_entity(std::move(link_name))))
        {}

        void add_member(std::unique_ptr<doc_cpp_entity> member)
        {
            if (size() == 0u)
            {
                set_comment(member->comment());
                member->entity().set_user_data(&peek());
            }
            member->group_member_no_ = unsigned(size() + 1u);

            add_child(std::move(member));
        }

    private:
        using basic_builder::add_child;
    };

private:
    doc_member_group_entity(std::string link_name) : doc_entity(std::move(link_name), nullptr) {}

    entity_kind do_get_kind() const noexcept override
    {
        return member_group;
    }

    markup::block_id do_get_id() const override
    {
        return markup::block_id(begin()->link_name());
    }

    std::unique_ptr<markup::documentation_entity> do_generate_documentation(
        const generation_config& gen_config, const synopsis_config& syn_config,
        const cppast::cpp_entity_index&                     index,
        type_safe::optional_ref<detail::inline_entity_list> inlines,
        std::unique_ptr<markup::code_block>                 synopsis) const override;

    cppast::code_generator::generation_options do_get_generation_options(
        const synopsis_config& config, bool is_main) const override;

    void do_generate_synopsis_prefix(const cppast::code_generator::output& output,
                                     const synopsis_config& config, bool is_main, type_safe::flag& needs_newline) const override;

    void do_generate_code(cppast::code_generator& generator) const override;
};

/// The documentation entity for namespaces.
///
/// This will be the user data of non-excluded [cppast::cpp_namespace]().
class doc_cpp_namespace final : public doc_entity
{
public:
    class builder : public doc_entity::basic_builder<doc_cpp_namespace>
    {
    public:
        builder(std::string link_name, type_safe::object_ref<const cppast::cpp_namespace> entity,
                type_safe::optional_ref<const comment::doc_comment> comment);
    };

    /// \returns The corresponding namespace.
    const cppast::cpp_namespace& namespace_() const noexcept
    {
        return *entity_;
    }

    /// \returns The incomplete namespace documentation.
    /// It is used for the [standardese::entity_index]().
    markup::namespace_documentation::builder get_builder() const;

private:
    doc_cpp_namespace(std::string                                         link_name,
                      type_safe::object_ref<const cppast::cpp_namespace>  entity,
                      type_safe::optional_ref<const comment::doc_comment> comment)
    : doc_entity(std::move(link_name), comment), entity_(entity)
    {}

    entity_kind do_get_kind() const noexcept override
    {
        return cpp_namespace;
    }

    markup::block_id do_get_id() const override
    {
        return markup::block_id(link_name());
    }

    std::unique_ptr<markup::documentation_entity> do_generate_documentation(
        const generation_config& gen_config, const synopsis_config& syn_config,
        const cppast::cpp_entity_index&                     index,
        type_safe::optional_ref<detail::inline_entity_list> inlines,
        std::unique_ptr<markup::code_block>                 synopsis) const override;

    cppast::code_generator::generation_options do_get_generation_options(
        const synopsis_config& config, bool is_main) const override;

    void do_generate_synopsis_prefix(const cppast::code_generator::output& code,
                                     const synopsis_config& config, bool is_main, type_safe::flag& needs_newline) const override;

    void do_generate_code(cppast::code_generator& generator) const override;

    type_safe::object_ref<const cppast::cpp_namespace> entity_;
};

/// The documentation entity representing a file.
///
/// This will be the matching entity of [cppast::cpp_file]().
class doc_cpp_file final : public doc_entity
{
public:
    /// Builds a file.
    class builder : public doc_entity::basic_builder<doc_cpp_file>
    {
    public:
        builder(std::string output_name, std::string link_name,
                std::unique_ptr<cppast::cpp_file>                   file,
                type_safe::optional_ref<const comment::doc_comment> comment);
    };

    /// \returns The corresponding file.
    /// It is owned by the doc entity.
    const cppast::cpp_file& file() const noexcept
    {
        return *file_;
    }

    /// \returns The output name of that file.
    const std::string& output_name() const noexcept
    {
        return output_name_;
    }

private:
    doc_cpp_file(std::string output_name, std::string link_name,
                 std::unique_ptr<cppast::cpp_file>                   file,
                 type_safe::optional_ref<const comment::doc_comment> comment)
    : doc_entity(std::move(link_name), comment), output_name_(std::move(output_name)),
      file_(std::move(file))
    {}

    entity_kind do_get_kind() const noexcept override
    {
        return cpp_file;
    }

    markup::block_id do_get_id() const override
    {
        return markup::block_id(link_name());
    }

    std::unique_ptr<markup::documentation_entity> do_generate_documentation(
        const generation_config& gen_config, const synopsis_config& syn_config,
        const cppast::cpp_entity_index&                     index,
        type_safe::optional_ref<detail::inline_entity_list> inlines,
        std::unique_ptr<markup::code_block>                 synopsis) const override;

    cppast::code_generator::generation_options do_get_generation_options(
        const synopsis_config& config, bool is_main) const override;

    void do_generate_code(cppast::code_generator& generator) const override;

    std::string                       output_name_;
    std::unique_ptr<cppast::cpp_file> file_;
};

class comment_registry;

/// Controls which entities are excluded in the documentation.
class entity_blacklist
{
public:
    /// \effects Creates a blacklist that blacklists private entities.
    entity_blacklist() : entity_blacklist(false) {}

    /// \effects Creates a blacklist that may blacklist private entities.
    explicit entity_blacklist(bool extract_private) : extract_private_(extract_private) {}

    /// \effects Blacklist a namespace name.
    /// It can either be a single name like `detail` or a nested one like `foo::bar`.
    void blacklist_namespace(std::string name)
    {
        ns_blacklist_.emplace(std::move(name));
    }

    /// \returns Whether or not the given entity is blacklisted according to this blacklist.
    bool is_blacklisted(const cppast::cpp_entity&         entity,
                        cppast::cpp_access_specifier_kind access) const;

private:
    std::unordered_set<std::string> ns_blacklist_;
    bool                            extract_private_;
};

/// Excludes all entities that need excluding.
/// \notes This must be called before [standardese::build_doc_entities]() for all files.
void exclude_entities(const comment_registry& registry, const cppast::cpp_entity_index& index,
                      const entity_blacklist& blacklist, const cppast::cpp_file& file);

/// Creates the [standardese::doc_entity]() hierarchy.
/// \effects Traverses over all entities in the file, builds matching doc entities and marks
/// excluded entities. \returns The corresponding documentation file. \notes The file output name is
/// merely a suggestion, may be overriden by comment of file.
std::unique_ptr<doc_cpp_file> build_doc_entities(
    type_safe::object_ref<const comment_registry> registry, const cppast::cpp_entity_index& index,
    std::unique_ptr<cppast::cpp_file> file, std::string output_name);
} // namespace standardese

#endif // STANDARDESE_DOC_ENTITY_HPP_INCLUDED
