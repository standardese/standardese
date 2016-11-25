// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_LINKER_HPP_INCLUDED
#define STANDARDESE_LINKER_HPP_INCLUDED

#include <mutex>
#include <unordered_map>

#include <standardese/md_inlines.hpp>

namespace standardese
{
    class doc_entity;
    class index;

    class linker
    {
    public:
        /// \effects Registers an external URL.
        /// All unresolved `unique-name`s starting with `prefix` will be resolved to `url`.
        /// If `url` contains two dollar signs (`$$`), this will be replaced by the (url-encoded) `unique-name`.
        void register_external(std::string prefix, std::string url);

        void register_entity(const doc_entity& e, std::string output_file) const;

        std::string register_anchor(const std::string& unique_name, std::string output_file) const;

        void change_output_file(const doc_entity& e, std::string output_file) const;

        std::string get_url(const index& idx, const doc_entity* context,
                            const std::string& unique_name, const char* extension) const;

        std::string get_url(const doc_entity& e, const char* extension) const;

        std::string get_anchor_id(const doc_entity& e) const;

        md_ptr<md_anchor> get_anchor(const doc_entity& e, const md_entity& parent) const;

    private:
        class location
        {
        public:
            location(const doc_entity& e, std::string output_file);

            location(const char* unique_name, std::string output_file);

            std::string format(const char* extension) const;

            void set_output_file(std::string output_file);

            const std::string& get_id() const STANDARDESE_NOEXCEPT
            {
                return id_;
            }

        private:
            std::string file_name_;
            std::string id_;
            bool        with_extension_;
        };

        mutable std::mutex mutex_;
        mutable std::unordered_map<const doc_entity*, location> locations_;
        mutable std::unordered_map<std::string, location>       anchors_;

        std::unordered_map<std::string, std::string> external_;
    };
} // namespace standardese

#endif // STANDARDESE_LINKER_HPP_INCLUDED
