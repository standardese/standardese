// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_METADATA_HPP_INCLUDED
#define STANDARDESE_COMMENT_METADATA_HPP_INCLUDED

#include <string>
#include <vector>

#include <type_safe/optional.hpp>
#include <type_safe/variant.hpp>

namespace standardese
{
    namespace comment
    {
        /// The exclude mode of an entity.
        enum class exclude_mode
        {
            entity,      //< The entire entity is excluded.
            return_type, //< The return type of a function is excluded.
            target       //< The target of an alias is excluded.
        };

        /// The group of an entity.
        class member_group
        {
        public:
            /// \effects Creates a group given the name,
            /// optional heading and whether or not the name is also an output section.
            member_group(std::string name, type_safe::optional<std::string> heading,
                         bool is_section)
            : heading_(std::move(heading)), name_(std::move(name)), is_section_(is_section)
            {
            }

            /// \returns The name of the group.
            const std::string& name() const noexcept
            {
                return name_;
            }

            /// \returns The heading of the group, if there is one.
            const type_safe::optional<std::string>& heading() const noexcept
            {
                return heading_;
            }

            /// \returns The output section of the group, if there is one.
            type_safe::optional_ref<const std::string> output_section() const noexcept
            {
                return is_section_ ? type_safe::optional_ref<const std::string>(name_) :
                                     type_safe::nullopt;
            }

        private:
            type_safe::optional<std::string> heading_;
            std::string                      name_;
            bool                             is_section_;
        };

        inline bool operator==(const member_group& a, const member_group& b)
        {
            return a.name() == b.name();
        }

        inline bool operator!=(const member_group& a, const member_group& b)
        {
            return !(a == b);
        }

        /// The metadata of a comment.
        ///
        /// It stores the information that can be set by commands.
        class metadata
        {
        public:
            /// \effects Creates it without any special settings.
            ///
            /// This is the metadata for a comment without any commands.
            metadata() = default;

            /// \effects Sets the exclude mode.
            /// \returns Whether it wasn't previously set.
            bool set_exclude(exclude_mode e) noexcept
            {
                auto set = exclude_.has_value();
                exclude_ = e;
                return !set;
            }

            /// \returns The exclude mode of the entity, if there is one.
            const type_safe::optional<exclude_mode>& exclude() const noexcept
            {
                return exclude_;
            }

            /// \effects Sets the unique name override.
            /// \returns whether it wasn't previously set.
            bool set_unique_name(std::string name)
            {
                auto set     = unique_name_.has_value();
                unique_name_ = std::move(name);
                return !set;
            }

            /// \returns The unique name override, if there is one.
            const type_safe::optional<std::string>& unique_name() const noexcept
            {
                return unique_name_;
            }

            /// \effects Sets the synopsis override.
            /// \returns whether it wasn't previously set.
            bool set_synopsis(std::string syn)
            {
                auto set  = synopsis_.has_value();
                synopsis_ = std::move(syn);
                return !set;
            }

            /// \returns The synopsis override, if there is one.
            const type_safe::optional<std::string>& synopsis() const noexcept
            {
                return synopsis_;
            }

            /// \effects Sets the group.
            /// \returns whether it wasn't previously set.
            bool set_group(member_group group)
            {
                auto set = group_.has_value();
                group_   = std::move(group);
                return !set;
            }

            /// \returns The group, if it is in one.
            const type_safe::optional<member_group>& group() const noexcept
            {
                return group_;
            }

            /// \effects Sets the module.
            /// \returns whether it wasn't previously set.
            bool set_module(std::string module)
            {
                auto set = module_.has_value();
                module_  = std::move(module);
                return !set;
            }

            /// \returns The name of the module, if there is one.
            const type_safe::optional<std::string>& module() const noexcept
            {
                return module_;
            }

            /// \effects Sets the output section.
            /// \returns whether it wasn't previously set.
            bool set_output_section(std::string section)
            {
                auto set = section_.has_value();
                section_ = std::move(section);
                return !set;
            }

            /// \returns The output section, if there is one.
            const type_safe::optional<std::string>& output_section() const noexcept
            {
                return section_;
            }

        private:
            type_safe::optional<member_group> group_;
            type_safe::optional<std::string>  unique_name_, synopsis_, module_, section_;
            type_safe::optional<exclude_mode> exclude_;
        };
    }
} // namespace standardese::comment

#endif // STANDARDESE_COMMENT_METADATA_HPP_INCLUDED
