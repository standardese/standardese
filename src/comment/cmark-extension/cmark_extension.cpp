// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <stdexcept>
#include <string>

#include <cmark-gfm.h>
#include <cmark-gfm-extension_api.h>

#include "cmark_extension.hpp"

namespace standardese::comment::cmark_extension {

void cmark_extension::cmark_node_insert_before(cmark_node* node, cmark_node* sibling) {
  if (::cmark_node_insert_before(node, sibling) == 0)
      throw std::logic_error("Sibling node " + to_xml(sibling) + " not allowed before node " + to_xml(node));
}

void cmark_extension::cmark_node_insert_after(cmark_node* node, cmark_node* sibling) {
  if (::cmark_node_insert_after(node, sibling) == 0)
      throw std::logic_error("Sibling node " + to_xml(sibling) + " not allowed after node " + to_xml(node));
}

void cmark_extension::cmark_node_replace(cmark_node* oldnode, cmark_node* newnode) {
  if (::cmark_node_replace(oldnode, newnode) == 0)
      throw std::logic_error("New node " + to_xml(newnode) + " not allowed as replacement for node " + to_xml(oldnode));
}

void cmark_extension::cmark_node_append_child(cmark_node* node, cmark_node* child) {
  if (::cmark_node_append_child(node, child) == 0)
      throw std::logic_error("Child node " + to_xml(child) + " not allowed in parent " + to_xml(node));
}

const char* cmark_extension::cmark_node_get_literal(cmark_node* node) {
  const char* literal = ::cmark_node_get_literal(node);
  if (literal == nullptr)
      throw std::logic_error("There is no literal content in node " + to_xml(node));

  return literal;
}

void cmark_extension::cmark_node_set_syntax_extension(cmark_node* node, cmark_syntax_extension* extension) {
  if (::cmark_node_set_syntax_extension(node, extension) == 0)
      throw std::logic_error("Failed to set syntax extension for node " + to_xml(node));
}

void cmark_extension::cmark_node_set_user_data(cmark_node* node, void* data) {
  if (::cmark_node_set_user_data(node, data) == 0)
      throw std::logic_error("Failed to set user data for node " + to_xml(node));
}

void cmark_extension::cmark_node_set_user_data_free_func(cmark_node* node, cmark_free_func func) {
  if (::cmark_node_set_user_data_free_func(node, func) == 0)
      throw std::logic_error("Failed to set function to free user data for node " + to_xml(node));
}

void cmark_extension::cmark_node_set_type(cmark_node* node, cmark_node_type type) {
  if (::cmark_node_set_type(node, type) == 0)
      throw std::logic_error("Failed to set node type for node " + to_xml(node));
}

std::string cmark_extension::to_xml(cmark_node* node) {
    if (node == nullptr)
        return "nullptr";

    cmark_mem* memory = cmark_get_default_mem_allocator();
    char* xml = cmark_render_xml_with_mem(node, 0, memory);
    std::string ret = xml;
    memory->free(xml);
    return ret;
}

}
