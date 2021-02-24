// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_COMMENT_CMARK_EXTENSION_CMARK_EXTENSION_HPP_INCLUDED
#define STANDARDESE_COMMENT_CMARK_EXTENSION_CMARK_EXTENSION_HPP_INCLUDED

#include <string>

#include <cmark-gfm.h>

namespace standardese::comment::cmark_extension
{
    /// Shared Base Class for Standardese's CommonMark Extensions.
    class cmark_extension
    {
      public:
        /// Call cmark's function of the same name and throw an exception if it reported an error.
        static void cmark_node_insert_before(cmark_node* node, cmark_node* sibling);

        /// Call cmark's function of the same name and throw an exception if it reported an error.
        static void cmark_node_insert_after(cmark_node* node, cmark_node* sibling);

        /// Call cmark's function of the same name and throw an exception if it reported an error.
        static void cmark_node_replace(cmark_node* oldnode, cmark_node* newnode);

        /// Call cmark's function of the same name and throw an exception if it reported an error.
        static void cmark_node_append_child(cmark_node* node, cmark_node* child);

        /// Call cmark's function of the same name and throw an exception if it reported an error.
        static const char* cmark_node_get_literal(cmark_node* node);

        /// Call cmark's function of the same name and throw an exception if it reported an error.
        static void cmark_node_set_syntax_extension(cmark_node* node, cmark_syntax_extension* extension);

        /// Call cmark's function of the same name and throw an exception if it reported an error.
        static void cmark_node_set_user_data(cmark_node* node, void* data);

        /// Call cmark's function of the same name and throw an exception if it reported an error.
        static void cmark_node_set_user_data_free_func(cmark_node* node, cmark_free_func func);

        /// Call cmark's function of the same name and throw an exception if it reported an error.
        static void cmark_node_set_type(cmark_node* node, cmark_node_type type);

        /// Return an XML representation of this CommonMark node (for debugging.)
        static std::string to_xml(cmark_node* node);
    };
}

#endif // STANDARDESE_COMMENT_CMARK_EXTENSION_CMARK_EXTENSION_HPP_INCLUDED
