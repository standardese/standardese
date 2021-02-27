// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_IGNORE_HTML_EXTENSION_IGNORE_HTML_EXTENSION_HPP_INCLUDED
#define STANDARDESE_COMMENT_IGNORE_HTML_EXTENSION_IGNORE_HTML_EXTENSION_HPP_INCLUDED

#include <cmark-gfm.h>
#include <cmark-gfm-extension_api.h>

#include <standardese/comment/config.hpp>

#include "../cmark-extension/cmark_extension.hpp"

namespace standardese::comment::ignore_html_extension
{
    /// A CommonMark Extenion to Treat HTML as Text.
    class ignore_html_extension : cmark_extension::cmark_extension {
      public:
        /// Create a IgnoreHTMLExtension and attach it to the parser.
        /// Return a reference to the created extension; to deallocate the
        /// extension, call cmark_syntax_extension_free() before
        /// cmark_parser_free()ing the parser itself.
        static ignore_html_extension& create(cmark_parser* parser);

        ~ignore_html_extension();

      private:
        ignore_html_extension(cmark_syntax_extension*);

        /// Process the tree of CommonMark nodes and turn any HTML nodes back
        /// into text nodes.
        static cmark_node* cmark_postprocess(cmark_syntax_extension*, cmark_parser*, cmark_node*);

        /// Process the CommonMark node and return it or the node it was turned into.
        static cmark_node* postprocess(cmark_node*);

        /// Merge `rhs` into `lhs` if both are text nodes.
        /// Return the surviving text node.
        static cmark_node* merge(cmark_node* lhs, cmark_node* rhs);
    };
}

#endif // STANDARDESE_COMMENT_IGNORE_HTML_EXTENSION_IGNORE_HTML_EXTENSION_HPP_INCLUDED
