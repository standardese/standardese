// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_class.hpp>

#include <cassert>
#include <vector>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/error.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

const char* standardese::to_string(cpp_access_specifier_t access) STANDARDESE_NOEXCEPT
{
    switch (access)
    {
    case cpp_private:
        return "private";
    case cpp_protected:
        return "protected";
    case cpp_public:
        return "public";
    }

    return "should never get here, Simba";
}

namespace
{
    cpp_access_specifier_t parse_access_specifier(cpp_cursor cur)
    {
        switch (clang_getCXXAccessSpecifier(cur))
        {
        case CX_CXXPrivate:
            return cpp_private;
        case CX_CXXProtected:
            return cpp_protected;
        case CX_CXXPublic:
            return cpp_public;
        default:
            break;
        }

        assert(false);
        throw parse_error(source_location(cur), "internal error");
    }
}

cpp_ptr<cpp_access_specifier> cpp_access_specifier::parse(translation_unit&, cpp_cursor cur,
                                                          const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_CXXAccessSpecifier);

    return detail::make_cpp_ptr<cpp_access_specifier>(cur, parent, parse_access_specifier(cur));
}

cpp_ptr<cpp_base_class> cpp_base_class::parse(translation_unit&, cpp_cursor cur,
                                              const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_CXXBaseSpecifier);

    auto name = detail::parse_class_name(cur);
    auto type = clang_getCursorType(cur);
    auto a    = parse_access_specifier(cur);
    auto virt = clang_isVirtualBase(cur);

    auto real_parent = standardese::get_class(parent);
    assert(real_parent);
    return detail::make_cpp_ptr<cpp_base_class>(cur, *real_parent,
                                                cpp_type_ref(std::move(name), type), a, !!virt);
}

cpp_name cpp_base_class::get_name() const
{
    return type_.get_name();
}

const cpp_class* cpp_base_class::get_class(const cpp_entity_registry& registry) const
    STANDARDESE_NOEXCEPT
{
    auto declaration = type_.get_declaration();

    auto entity = registry.try_lookup(declaration);
    if (!entity)
        return nullptr;

    auto c = standardese::get_class(*entity);
    assert(c);
    return c;
}

const cpp_entity* cpp_base_class::do_get_semantic_parent() const STANDARDESE_NOEXCEPT
{
    assert(has_ast_parent());
    auto c = standardese::get_class(get_ast_parent());
    assert(c);
    return c->is_templated() ? &c->get_ast_parent() : c;
}

namespace
{
    cpp_class_type parse_class_type(cpp_cursor cur)
    {
        auto kind = clang_getCursorKind(cur);
        if (kind == CXCursor_ClassTemplate || kind == CXCursor_ClassTemplatePartialSpecialization)
            kind = clang_getTemplateCursorKind(cur);

        if (kind == CXCursor_ClassDecl)
            return cpp_class_t;
        else if (kind == CXCursor_StructDecl)
            return cpp_struct_t;
        else if (kind == CXCursor_UnionDecl)
            return cpp_union_t;

        assert(!"invalid cursor type for cpp_class");
        return cpp_class_t;
    }

    bool parse_class(translation_unit& tu, cpp_cursor cur, bool& is_final,
                     std::string& specialization_name)
    {
        detail::tokenizer tokenizer(tu, cur);
        auto              stream = detail::make_stream(tokenizer);
        auto              name   = detail::parse_name(cur);

        // handle extern templates
        if (detail::skip_if_token(stream, "extern"))
            return false;

        if (detail::skip_if_token(stream, "template"))
        {
            // skip until class name
            while (stream.peek().get_value() != name.c_str())
                stream.bump();
        }
        else
        {
            // skip class/struct/union/keyword
            stream.bump();
            detail::skip_whitespace(stream);
            detail::skip_attribute(stream, cur);
            detail::skip_whitespace(stream);
        }
        detail::skip(stream, cur, {name.c_str()});

        // we need to go backwards from the end
        // but the iterator doesn't support that, so store them
        std::vector<detail::token_stream::iterator> tokens;
        for (; !stream.done(); stream.bump())
        {
            if (stream.peek().get_value() == ";")
                // end of class declaration
                return false;
            else if (stream.peek().get_value() == ":")
                // beginning of bases
                break;
            else if (stream.peek().get_value() == "{"
                     && (tokens.empty() || *(std::prev(tokens.back()->get_value().end())) == '>'))
                // beginning of class definition
                // just rudimentary check against uniform initialization inside the args
                break;
            else if (!std::isspace(stream.peek().get_value()[0]))
                tokens.push_back(stream.get_iter());
        }

        is_final = false;
        for (auto iter = tokens.rbegin(); iter != tokens.rend(); ++iter)
        {
            auto& token = *iter;
            auto& str   = token->get_value();

            if (str == "final")
                is_final = true;
            else if (str == ">" || str == ">>")
            {
                // end of template arguments
                // concatenate them all
                stream.reset(tokens.front());
                specialization_name = name.c_str();
                while (stream.get_iter() != *std::prev(iter.base()))
                    specialization_name += stream.get().get_value().c_str();
                specialization_name += '>';
                break;
            }
        }

        return true;
    }
}

cpp_ptr<cpp_class> cpp_class::parse(translation_unit& tu, cpp_cursor cur, const cpp_entity& parent)
{
    auto ctype = parse_class_type(cur);

    auto        is_final = false;
    std::string args;
    auto        definition = parse_class(tu, cur, is_final, args);
    if (!definition)
        return nullptr;

    if (parent.get_entity_type() == cpp_entity::class_template_full_specialization_t
        && parent.get_cursor() == cur)
    {
        assert(!args.empty());
        auto& non_const = const_cast<cpp_entity&>(parent); // save here
        auto& templ     = static_cast<cpp_class_template_full_specialization&>(non_const);
        templ.name_     = std::move(args);
    }
    else if (parent.get_entity_type() == cpp_entity::class_template_partial_specialization_t
             && parent.get_cursor() == cur)
    {
        assert(!args.empty());
        auto& non_const = const_cast<cpp_entity&>(parent); // save here
        auto& templ     = static_cast<cpp_class_template_partial_specialization&>(non_const);
        templ.name_     = std::move(args);
    }
    else
        assert(args.empty());

    return detail::make_cpp_ptr<cpp_class>(cur, parent, ctype, is_final);
}

bool cpp_class::is_templated() const STANDARDESE_NOEXCEPT
{
    assert(has_ast_parent());
    return is_type_template(get_ast_parent().get_entity_type())
           && get_ast_parent().get_cursor() == get_cursor();
}

bool standardese::is_base_of(const cpp_entity_registry& registry, const cpp_class& base,
                             const cpp_class& derived) STANDARDESE_NOEXCEPT
{
    if (base.get_name() == derived.get_name())
        // same non-union class
        return base.get_class_type() != cpp_union_t;
    else if (base.is_final())
        return false;

    for (auto& cur_base : derived.get_bases())
    {
        if (base.get_name() == cur_base.get_name())
            // cur_base is equal to base
            return true;

        auto cur_base_class = cur_base.get_class(registry);
        if (cur_base_class && is_base_of(registry, base, *cur_base_class))
            // we know more about the current base class
            // and base is a base of cur_base_class
            // so an indirect base of derived
            return true;
    }

    return false;
}
