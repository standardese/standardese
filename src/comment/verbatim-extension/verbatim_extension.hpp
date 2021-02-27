// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_VERBATIM_EXTENSION_VERBATIM_EXTENSION_HPP_INCLUDED
#define STANDARDESE_COMMENT_VERBATIM_EXTENSION_VERBATIM_EXTENSION_HPP_INCLUDED

#include <cmark-gfm.h>
#include <cmark-gfm-extension_api.h>

#include <standardese/comment/config.hpp>

#include "../cmark-extension/cmark_extension.hpp"

namespace standardese::comment::verbatim_extension
{
    /// A CommonMark Extenion to Support the `\verbatim` Command.
    /// Comments marked as `\verbatim` will be completely ignored by the rules
    /// of CommonMark (and Standardese) and put into the output verbatim.
    class verbatim_extension : cmark_extension::cmark_extension {
      public:
        /// Create a VerbatimExtension and attach it to the parser.
        /// Return a reference to the created extension; to deallocate the
        /// extension, call cmark_syntax_extension_free() before
        /// cmark_parser_free()ing the parser itself.
        static verbatim_extension& create(cmark_parser* parser);

        /// Return the type for a node created by this extension.
        static cmark_node_type node_type();

        ~verbatim_extension();

      private:
        verbatim_extension(cmark_syntax_extension*);

        /// Return a string representation of the node that was created by this extension.
        /// Only relevant for extension debugging.
        static const char* cmark_get_type_string(cmark_syntax_extension*, cmark_node*);

        /// Called when a `\` is found in a comment.
        /// If this is part of a `\verbatim` command, returns a verbatim node,
        /// otherwise a nullptr.
        static cmark_node* cmark_match_inline(cmark_syntax_extension*, cmark_parser*, cmark_node*, unsigned char, cmark_inline_parser*);
    };
}

#endif // STANDARDESE_COMMENT_VERBATIM_EXTENSION_VERBATIM_EXTENSION_HPP_INCLUDED
