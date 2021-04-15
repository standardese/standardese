// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
//                    2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_COMMAND_EXTENSION_COMMAND_EXTENSION_HPP_INCLUDED
#define STANDARDESE_COMMENT_COMMAND_EXTENSION_COMMAND_EXTENSION_HPP_INCLUDED

#include <optional>
#include <functional>

#include <cmark-gfm.h>

#include <standardese/comment/config.hpp>

#include "../cmark-extension/cmark_extension.hpp"

namespace standardese::comment::command_extension
{
    /// A CommonMark Extension to use Standardese Commands in Markdown.
    /// This extension adds the commands supported by Standardese to a cmark
    /// parser, except for the verbatim command which is provided by the
    /// verbatim_extension.
    class command_extension : cmark_extension::cmark_extension
    {
      public:
        /// Create a CommandExtension and attach it to the parser.
        /// Return a reference to the created extension; to deallocate the
        /// extension, call cmark_syntax_extension_free() before
        /// cmark_parser_free()ing the parser itself.
        static command_extension& create(cmark_parser* parser, const class config&);

        /// Return the type for an extension node where `T` is either
        /// * `command_type`, for a special command such as `\exclude`
        /// * `section_type`, for a section such as `\brief`
        /// * `inline_type`, for a node such as `\param` that lives "inlined"
        ///   in the documentation of another node.
        template <typename T>
        static cmark_node_type node_type();

        ~command_extension();

      private:
        command_extension(const config&, cmark_syntax_extension*);

        /// Return a string representation of the node that was created by this
        /// extension. Only relevant for extension debugging.
        static const char* cmark_get_type_string(cmark_syntax_extension*, cmark_node*);

        /// Return whether `node` (which was created by this extension) can
        /// contain this `child`.
        /// While this function is also used by cmark to sanity check the tree
        /// of nodes, its main purpose is to decide how blocks should be
        /// nested when building the tree of nodes, i.e., when cmark parses
        /// ```
        /// \returns
        /// A return value.
        /// ```
        /// We need to pretend that the `<section>` corresponding to the
        /// `\returns` can not contain a paragraph since otherwise, cmark would
        /// immediately, put the `A return value.` paragraph into the
        /// `<section>` node. However, we want full control over this and only
        /// do the nesting later in the postprocessing step. Therefore, we
        /// initially tell cmark that our nodes cannot contain anything but
        /// during postprocessing change our mind on this as we nest our nodes
        /// manually.
        static int cmark_can_contain(cmark_syntax_extension*, cmark_node* parent_node, cmark_node_type child);

        /// Create a new node corresponding to one of our supported commands in
        /// `parent_container` and return it.
        static cmark_node* cmark_open_block(cmark_syntax_extension*, int indent, cmark_parser*, cmark_node* parent_container, unsigned char *input, int len);

        /// Postprocess the tree of nodes under the node root when the parsing is complete.
        /// This turns the list of nodes that the parser found into an actual tree of nodes, e.g., 
        /// `\returns x` which was parsed into something ressembling
        /// `<section /><paragraph>x</paragraph>` is turned into
        /// `<section><paragraph>x</paragraph></section>`.
        /// At the same time this might introduce an implicit "brief" node
        /// that is generated from the first line of the description if no
        /// brief was provided explicitly.
        /// \returns The new root of the syntax tree, or a nullptr if the root should be unchanged.
        static cmark_node* cmark_postprocess(cmark_syntax_extension*, cmark_parser* parser, cmark_node* root);

        cmark_node* postprocess(cmark_node* root) const;
        cmark_node* postprocess_paragraph(cmark_node* paragraph, std::optional<cmark_node*>& brief, cmark_node*& details) const;
        cmark_node* postprocess_block(cmark_node* block, cmark_node*& details) const;
        cmark_node* postprocess_inline_command(cmark_node* command) const;
        cmark_node* postprocess_section_command(cmark_node* command, std::optional<cmark_node*>& brief, cmark_node*& details) const;

        cmark_node* create_section_node(section_type) const;
        cmark_node* create_inline_node(inline_type) const;

        /// Split this `paragraph` at the first node satisfying `is_end`. Keep
        /// everything before the end node untouched, and wrap everything
        /// after and including the end node into a single sibling paragraph.
        /// Returns the next sibling of `paragraph`, i.e., the one that was
        /// created during the split.
        cmark_node* split_paragraph(cmark_node* paragraph, std::function<bool(cmark_node*)> is_end) const;

        /// Starting at `begin`, move all siblings up to but excluding `end`
        /// into `target`. When `end` is a nullptr, move all of the siblings.
        void splice(cmark_node* target, cmark_node* begin, cmark_node* end) const;

        /// Starting at `begin`, move all siblings up to a section end into
        /// `target`, i.e., up to an explicit `\end` command or some implicit
        /// section end. Drop any explicit `\end` nodes and return the first
        /// node that has not been moved.
        cmark_node* splice(cmark_node* target, cmark_node* begin) const;

        /// Perform some generic cleanup on a node, e.g., by removing trailing
        /// line breaks and return the cleaned up node or a nullptr if the node
        /// was deemed trivially empty after the cleanup (and therefore deleted.)
        cmark_node* cleanup(cmark_node*) const;

        /// Return whether this node ends an implicit brief, e.g., because it
        /// is the softbreak following a full stop.
        bool is_brief_end(cmark_node*) const;

        /// Return whether this node explicitly ends a section, e.g., because
        /// it is a command starting a new section.
        bool is_explicit_section_end(cmark_node*) const;

        /// Return whether this node ends a section explicitly, e.g., because
        /// it is a command starting a new section, or implicitly, e.g., because
        /// it is the end of a paragraph.
        bool is_section_end(cmark_node*) const;

        /// Return whether this `parent` node can contain any of our standardese specific commands.
        static bool can_contain_command(cmark_node* parent);

        /// Return a single node parsed from the comment starting at `begin` from the string `[begin, end)`.
        /// This will also advance the pointer `begin` for the characters that have been read.
        cmark_node* parse_command(cmark_parser* parser, cmark_node* parent_container, unsigned char*& begin, unsigned char* end, int indent);

        const comment::config& config_;

        /// The underlying cmark extension that provides the C interface to this class.
        cmark_syntax_extension* extension_;

        /// Whether the postprocessing has started already.
        /// \see [*cmark_can_contain]() for why we need to keep track of this.
        bool postprocessing = false;
    };
}

#endif // STANDARDESE_COMMENT_COMMAND_EXTENSION_COMMAND_EXTENSION_HPP_INCLUDED
