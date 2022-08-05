// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "command_extension.hpp"
#include "user_data.hpp"
#include "../../util/enum_values.hpp"

#include <type_traits>
#include <cassert>
#include <cstring>

#include <cmark-gfm.h>
#include <cmark-gfm-extension_api.h>

namespace standardese::comment::command_extension
{

command_extension::command_extension(const class config& config, cmark_syntax_extension* extension) : config_(config), extension_(extension)
{
    cmark_syntax_extension_set_get_type_string_func(extension, command_extension::cmark_get_type_string);
    cmark_syntax_extension_set_can_contain_func(extension, command_extension::cmark_can_contain);
    cmark_syntax_extension_set_open_block_func(extension, cmark_open_block);
    cmark_syntax_extension_set_postprocess_func(extension, cmark_postprocess);
}

command_extension::~command_extension() {}

template <typename T>
cmark_node_type command_extension::node_type() {
    static const auto type = cmark_syntax_extension_add_node(0);
    return type;
}

const char* command_extension::cmark_get_type_string(cmark_syntax_extension* extension, cmark_node* node){
  const auto type = cmark_node_get_type(node);
  if (type == node_type<command_type>())
      return "special-command";
  if (type == node_type<inline_type>())
      return "inline";
  if (type == node_type<section_type>())
      return "section";

  return "<unknown>";
}

int command_extension::cmark_can_contain(cmark_syntax_extension* extension, cmark_node* parent_node, cmark_node_type child)
{
    command_extension& self = *static_cast<command_extension*>(cmark_syntax_extension_get_private(extension));

    if (!self.postprocessing)
        // We don't want cmark to do any automatic nesting. We do this
        // ourselves during the postprocessing.
        return false;

    const auto parent = cmark_node_get_type(parent_node);

    if (parent == node_type<command_type>())
    {
        // No nodes can be nested inside a command.
        return false;
    }

    if (parent == node_type<section_type>())
    {
        // Sections can contain any standard markdown blocks.
        if ((child & CMARK_NODE_TYPE_MASK) == CMARK_NODE_TYPE_BLOCK)
            return true;
        return false;
    }

    if (parent == node_type<inline_type>())
    {
        // Inlines can contain brief & details.
        if (cmark_node_type(child) == node_type<section_type>())
            return true;
        // Inlines can contain some special commands such as \exclude or \module.
        if (cmark_node_type(child) == node_type<command_type>())
            return true;

        return false;
    }

    throw std::logic_error("not implemented: unknown parent node type");
}

cmark_node* command_extension::cmark_open_block(cmark_syntax_extension* extension, int indent, cmark_parser* parser, cmark_node* parent_container, unsigned char *input, int len)
{
    command_extension& self = *static_cast<command_extension*>(cmark_syntax_extension_get_private(extension));

    assert(!self.postprocessing && "Once the postprocessing has started we cannot parse further blocks as this would break the way blocks are nested.");

    if (!can_contain_command(parent_container))
        return nullptr;

    const int offset = cmark_parser_get_offset(parser);

    unsigned char* begin = input + offset;
    assert(begin <= input + len);

    auto node = self.parse_command(parser, parent_container, begin, input + len, indent);
    assert(begin >= input + offset && begin <= input + len);

    cmark_parser_advance_offset(parser, reinterpret_cast<char*>(input), begin - (input + offset), false);

    return node;
}

cmark_node* command_extension::cmark_postprocess(cmark_syntax_extension* extension, cmark_parser* parser, cmark_node* root)
{
    command_extension& self = *static_cast<command_extension*>(cmark_syntax_extension_get_private(extension));

    self.postprocessing = true;

    return self.postprocess(root);
}

cmark_node* command_extension::postprocess(cmark_node* root) const
{
    if (root == nullptr)
        // An empty document, nothing to postprocess.
        return nullptr;

    if (cmark_node_get_type(root) == CMARK_NODE_DOCUMENT) {
        // Everything that standardese sees lives under a single <document> node created by cmark.
        assert(cmark_node_next(root) == nullptr && "expected all nodes to be children of a single <document> node.");

        const auto* next = postprocess(cmark_node_first_child(root));

        assert(next == nullptr && "postprocess() must process all children of a document and we did not expect more than a <document> at the root level.");
        return nullptr;
    }

    // We now process the root node and all of its siblings, turning them into
    // properly nested trees of nodes.
    // Any stray chunks of text will be understood as "details" for the root
    // node, and the first line of these details will be used as the "brief"
    // unless a \brief has been explicitly specified.
    std::optional<cmark_node*> brief = std::nullopt;
    cmark_node* details = nullptr;

    for (auto* sibling = root; sibling != nullptr;)
    {
        const auto type = cmark_node_get_type(sibling);

        if (type == CMARK_NODE_PARAGRAPH) {
            sibling = postprocess_paragraph(sibling, brief, details);
        } else if (type == node_type<command_type>()) {
            // This is some other command such as \output_section.
            // We do not need to collect any children for it here so we leave it alone.
            sibling = cmark_node_next(sibling);
        } else {
            if (!brief) {
                // The first line of the comment could have defined an implicit
                // brief. But a brief cannot be formed (implicitly) after any
                // section or inline has been given. So there is no implicit
                // brief anymore.
                brief = nullptr;
            }

            if (type == node_type<section_type>()) {
                // This is a section-command such as \returns. We need to
                // collect the parts of this section which are currently
                // siblings and turn them into children.
                sibling = postprocess_section_command(sibling, brief, details);
            } else if (type == node_type<inline_type>()) {
                // This is an inline-command, i.e., documentation for
                // another entity, such as \param. We need to collect its
                // children which are currently siblings.
                sibling = postprocess_inline_command(sibling);
            } else if (
                // Supported Markdown Blocks
                type == CMARK_NODE_BLOCK_QUOTE ||
                type == CMARK_NODE_LIST ||
                type == CMARK_NODE_CODE_BLOCK ||
                type == CMARK_NODE_HEADING ||
                type == CMARK_NODE_THEMATIC_BREAK) {
                sibling = postprocess_block(sibling, details);
            } else {
                throw std::logic_error("unknown sibling node type at root of: " + to_xml(sibling));
            }
        }
    }

    return nullptr;
}

bool command_extension::is_brief_end(cmark_node* node) const {
     // .!? at the end of the line ends an implicit brief.
     if (cmark_node_get_type(node) == CMARK_NODE_SOFTBREAK &&
         cmark_node_get_type(cmark_node_previous(node)) == CMARK_NODE_TEXT) {
         const char* previous = cmark_node_get_literal(cmark_node_previous(node));
         const auto length = std::strlen(previous);
         assert(length > 0 && "a SOFTBREAK must follow text otherwise it would be a LINEBREAK.");
         const char last = previous[length - 1];
         if (last == '.' || last == '?' || last == '!') return true;
     }
     // Anything that would end an explicit section also ends an implicit (brief) section.
     if (is_section_end(node))
         return true;

     return false;
}

cmark_node* command_extension::postprocess_paragraph(cmark_node* paragraph, std::optional<cmark_node*>& brief, cmark_node*& details) const {
    if (!brief) {
        // Split the paragraph into an implicit brief (typically the first line) and more details (remaining lines)
        cmark_node* more = split_paragraph(paragraph, [&](cmark_node* node){ return is_brief_end(node); });

        if (cleanup(paragraph) == nullptr) {
            // The brief is empty, e.g., an empty line.
            brief = nullptr;
        } else {
            // Now paragraph contains the implicit brief.
            // So we create a brief node and move the paragraph into it.
            brief = create_section_node(section_type::brief);
            cmark_node_insert_before(paragraph, *brief);
            cmark_node_append_child(*brief, paragraph);
        }

        // We will likely come back to this method to process the more
        // paragraph with the else{} block that follows.
        return more;
    } else {
        // There is already a brief (or for some other reason we cannot have a
        // brief anymore.) So these are details.
        if (details == nullptr) {
            // If there are no details yet, create an empty details node.
            details = create_section_node(section_type::details);
            cmark_node_insert_before(paragraph, details);
        }

        // We take the first bit of this paragraph and put it into the details.
        cmark_node* more = split_paragraph(paragraph, [&](cmark_node* end) {
            return is_section_end(end);
        });

        if (cleanup(paragraph) != nullptr)
            cmark_node_append_child(details, paragraph);

        return more;
    }
}

cmark_node* command_extension::cleanup(cmark_node* node) const {
    const auto is_trivial = [](cmark_node* node) {
        if (cmark_node_get_type(node) == CMARK_NODE_LINEBREAK) return true;
        if (cmark_node_get_type(node) == CMARK_NODE_SOFTBREAK) return true;

        return false;
    };

    if (node == nullptr) {
        return nullptr;
    } else if (cmark_node_get_type(node) == CMARK_NODE_PARAGRAPH) {
        // Drop leading/trailing newlines from paragraphs.
        while (cmark_node* child = cmark_node_first_child(node)) {
            if (is_trivial(child)) cmark_node_free(child);
            else break;
        }
        while (cmark_node* child = cmark_node_last_child(node)) {
            if (is_trivial(child)) cmark_node_free(child);
            else break;
        }

        // Drop an empty paragraph.
        if (cmark_node_first_child(node) == nullptr) {
            cmark_node_free(node);
            node = nullptr;
        }
    } else {
        throw std::logic_error("not implemented: cleanup of this kind of node");
    }

    return node;
}

cmark_node* command_extension::postprocess_block(cmark_node* block, cmark_node*& details) const {
    if (details == nullptr) {
        // If there are no details yet, create an empty details node.
        details = create_section_node(section_type::details);
        cmark_node_insert_before(block, details);
    }

    // We do not process what is inside the block since no standardese-specific
    // commands can be nested inside. So we add the block to the details as is.
    cmark_node* next = cmark_node_next(block);
    cmark_node_append_child(details, block);
    return next;
}

bool command_extension::is_explicit_section_end(cmark_node* node) const {
    if (node == nullptr)
        // When there are no more siblings, a section ends.
        return true;
    // Any section command ends the preceding section.
    if (cmark_node_get_type(node) == node_type<section_type>())
        return true;
    // Any description of another entity ends the preceding section.
    if (cmark_node_get_type(node) == node_type<inline_type>())
        return true;
    // Many special commands ends the preceding section.
    if (cmark_node_get_type(node) == node_type<command_type>()) {
        const auto& parsed = user_data<command_type>::get(node);
        switch(parsed.command) {
            case command_type::exclude:
            case command_type::module:
                return false;
            default:
                return true;
        }
    }

    return false;
}

bool command_extension::is_section_end(cmark_node* node) const {
    if (is_explicit_section_end(node))
        return true;

    // A hard linebreak ends a section, i.e., a backslash at the end of the line in Markdown.
    if (cmark_node_get_type(node) == CMARK_NODE_LINEBREAK)
        return true;

    // The end of a paragraph ends a section.
    cmark_node* previous = cmark_node_previous(node);
    if (previous != nullptr && cmark_node_get_type(previous) == CMARK_NODE_PARAGRAPH)
        return true;

    return false;
}

cmark_node* command_extension::create_section_node(section_type kind) const {
  cmark_node* node = cmark_node_new(node_type<section_type>());
  cmark_node_set_syntax_extension(node, extension_);
  user_data<section_type>::set(node, kind, {});

  return node;
}

cmark_node* command_extension::create_inline_node(inline_type kind) const {
  cmark_node* node = cmark_node_new(node_type<inline_type>());
  cmark_node_set_syntax_extension(node, extension_);
  user_data<inline_type>::set(node, kind, {});

  return node;
}

cmark_node* command_extension::split_paragraph(cmark_node* paragraph, std::function<bool(cmark_node*)> is_end) const {
    // Find the first end node satisfying is_end
    cmark_node* end = cmark_node_first_child(paragraph);
    if (end != nullptr) end = cmark_node_next(end);
    while (end != nullptr && !is_end(end))
      end = cmark_node_next(end);

    if (end != nullptr) {
        // Create a new paragraph to hold everything that used to be after the end node.
        cmark_node* new_paragraph = cmark_node_new(CMARK_NODE_PARAGRAPH);
        cmark_node_insert_after(paragraph, new_paragraph);
        splice(new_paragraph, end, nullptr);
    }

    return cmark_node_next(paragraph);
}

void command_extension::splice(cmark_node* target, cmark_node* begin, cmark_node* end) const {
    while (begin != end) {
        cmark_node* move = begin;
        begin = cmark_node_next(begin);
        cmark_node_append_child(target, move);
    }
}

cmark_node* command_extension::postprocess_section_command(cmark_node* command, std::optional<cmark_node*>& brief, cmark_node*& details) const
{
    const auto& data = user_data<section_type>::get(command);

    cmark_node* target = command;

    switch (data.command)
    {
        case section_type::brief:
            // This command starts an explicit \brief section. We create a
            // new brief node unless there is already one.
            if (!brief || brief == nullptr) {
                brief = command;
            } else {
                cmark_node_free(command);
            }
            // Append everything in this section to the brief node.
            target = *brief;
            break;
        case section_type::details:
            // This command starts an explicit \details section. We create a
            // details node unless there is already one.
            if (details == nullptr) {
                details = command;
            } else {
                cmark_node_free(command);
            }
            // Append everything in thi section to the details node.
            target = details;
            break;
        case section_type::requires:
        case section_type::effects:
        case section_type::synchronization:
        case section_type::postconditions:
        case section_type::returns:
        case section_type::throws:
        case section_type::complexity:
        case section_type::remarks:
        case section_type::error_conditions:
        case section_type::notes:
        case section_type::preconditions:
        case section_type::constraints:
        case section_type::diagnostics:
        case section_type::see:
            break;
        default:
            throw std::logic_error("unsupported section type could not be postprocessed");
    }

    // Move the contents of this section into the target node.
    cmark_node* more = splice(target, cmark_node_next(command));
    return more;
}

cmark_node* command_extension::splice(cmark_node* target, cmark_node* begin) const
{
    // We search for the end of this section.
    cmark_node* end = begin;
    while (!is_explicit_section_end(end))
        end = cmark_node_next(end);

    if (end != nullptr &&
        cmark_node_get_type(end) == node_type<command_type>() &&
        user_data<command_type>::get(end).command == command_type::end) {
        // When there is an explicit \end command for this section we will copy
        // everything up to that end command but delete the \end node itself.
        cmark_node* drop = end;
        end = cmark_node_next(end);
        cmark_node_free(drop);
    } else {
        // When there is no explicit \end, this section might end implicitly
        // before the next section starts and everything in between is part of
        // the details again.

        end = begin;
        while (!is_section_end(end)) {
            // When this section would end with a paragraph, not all of it
            // might go into this section as there might be a reason to end
            // that section inside that paragraph.
            if (end != nullptr && cmark_node_get_type(end) == CMARK_NODE_PARAGRAPH) {
                end = split_paragraph(end, [&](cmark_node* node) { return is_section_end(node); });
                break;
            }
            end = cmark_node_next(end);
        }
    }

    splice(target, begin, end);
    return end;
}

cmark_node* command_extension::postprocess_inline_command(cmark_node* command) const
{
    // Collect the contents of this inline and move them temoporarily into a
    // separate document for further postprocessing. (Since the rules of
    // postprocessing an inline are the same than the rules of postprocessing
    // an entire comment.)
    cmark_node* document = cmark_node_new(CMARK_NODE_DOCUMENT);
    cmark_node* more = splice(document, cmark_node_next(command));

    // Split the contents into brief & details sections and move them back the newly created innline node.
    postprocess(cmark_node_first_child(document));
    splice(command, cmark_node_first_child(document), nullptr);

    cmark_node_free(document);

    return more;
}

command_extension& command_extension::create(cmark_parser* parser, const comment::config& config)
{
    auto* cmark_extension = cmark_syntax_extension_new("standardese_commands");
    auto* extension = new command_extension(config, cmark_extension);
    cmark_syntax_extension_set_private(
        cmark_extension,
        extension,
        [](cmark_mem*, void* data) {
          delete static_cast<command_extension*>(data);
        }
    );
    cmark_parser_attach_syntax_extension(parser, cmark_extension);
    return *extension;
}

bool command_extension::can_contain_command(cmark_node* parent_node)
{
    const auto parent = cmark_node_get_type(parent_node);
    const auto parent_parent = cmark_node_get_type(cmark_node_parent(parent_node));

    if (parent == CMARK_NODE_DOCUMENT || parent == CMARK_NODE_LIST || parent == node_type<inline_type>())
        return true;

    // Allow commands in paragraphs if that paragraph's parent is either:
    // * document (happens when command on second line in paragraph)
    // * inline (happens when command in inline)
    if (parent == CMARK_NODE_PARAGRAPH)
        return parent_parent == CMARK_NODE_DOCUMENT || parent_parent == node_type<inline_type>();

    return false;
}

cmark_node* command_extension::parse_command(cmark_parser* parser, cmark_node* parent_container, unsigned char*& begin, unsigned char* end, int indent)
{
    // Try to parse a "command" node into "node".
    const auto parse_command = [&](const auto command) -> cmark_node* {
        using type = std::remove_cv_t<decltype(command)>;

        const auto& pattern = config_.get_command_pattern(command);

        std::match_results<char*> match;
        if (!std::regex_search(reinterpret_cast<char*>(begin), reinterpret_cast<char*>(end), match, pattern, std::regex_constants::match_continuous))
            return nullptr;

        begin += match.length();

        // We found a match for this command. Create a node for it and store
        // the command type and the match in it. We'll process the arguments later.
        cmark_node* node = cmark_parser_add_child(parser, parent_container, node_type<type>(), indent);

        cmark_node_set_syntax_extension(node, extension_);
        cmark_node_set_string_content(node, nullptr);
        user_data<type>::set(node, command, std::vector<std::string>(++std::begin(match), std::end(match)));

        return node;
    };

    // We inefficiently, try to match every command regex here. This could be
    // done much more efficiently, e.g., by discarding any leading characters
    // that fail to produce a partial match. But presumably, constructing the
    // C++ AST in the first place is the relevant bottleneck anyway.
    for (const auto command : enum_values<command_type>())
        if (cmark_node* node = parse_command(command))
            return node;
    for (const auto command : enum_values<section_type>())
        if (cmark_node* node = parse_command(command))
            return node;
    for (const auto command : enum_values<inline_type>())
        if (cmark_node* node = parse_command(command))
            return node;

    return nullptr;
}

// Explicitly instantiate templates for the linker.
template cmark_node_type command_extension::node_type<inline_type>();
template cmark_node_type command_extension::node_type<command_type>();
template cmark_node_type command_extension::node_type<section_type>();

}
