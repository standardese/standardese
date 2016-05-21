// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_class.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/parser.hpp>
#include <standardese/detail/tokenizer.hpp>

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
   cpp_access_specifier_t parse_access_specifier(CX_CXXAccessSpecifier a)
   {
       switch (a)
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
   }
}

cpp_ptr<cpp_access_specifier> cpp_access_specifier::parse(translation_unit &, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_CXXAccessSpecifier);

    return detail::make_ptr<cpp_access_specifier>(parse_access_specifier(clang_getCXXAccessSpecifier(cur)));
}

cpp_ptr<cpp_base_class> cpp_base_class::parse(translation_unit &, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_CXXBaseSpecifier);

    auto name = detail::parse_class_name(cur);
    auto type = clang_getCursorType(cur);
    auto a = parse_access_specifier(clang_getCXXAccessSpecifier(cur));
    auto virt = clang_isVirtualBase(cur);

    return detail::make_ptr<cpp_base_class>(std::move(scope), std::move(name), type, a, virt != 0);
}

namespace
{
    bool parse_class(translation_unit &tu, cpp_cursor cur,
                     const cpp_name &name, bool &is_final)
    {
        detail::tokenizer tokenizer(tu, cur);

        auto stream = detail::make_stream(tokenizer);
        source_location location(clang_getCursorLocation(cur), name);

        // handle extern templates
        if (detail::skip_if_token(stream, "extern"))
            return false;

        if (detail::skip_if_token(stream, "template"))
        {
            detail::skip_bracket_count(stream, location, "<", ">");
            detail::skip_whitespace(stream);
        }

        // skip class/struct/union/keyword and name
        stream.bump();
        detail::skip_whitespace(stream);
        detail::skip(stream, location, {name.c_str()});

        if (stream.peek().get_value() == "<")
        {
            detail::skip_bracket_count(stream, location, "<", ">");
            detail::skip_whitespace(stream);
        }

        if (stream.peek().get_value() == "final")
        {
            stream.bump();
            detail::skip_whitespace(stream);
            is_final = true;
        }
        else
            is_final = false;

        return stream.peek().get_value() != ";";
    }
}

cpp_class::parser::parser(translation_unit &tu, cpp_name scope, cpp_cursor cur)
{
    cpp_class_type ctype;

    auto kind = clang_getCursorKind(cur);
    if (kind == CXCursor_ClassTemplate
        || kind == CXCursor_ClassTemplatePartialSpecialization)
        kind = clang_getTemplateCursorKind(cur);

    if (kind == CXCursor_ClassDecl)
        ctype = cpp_class_t;
    else if (kind == CXCursor_StructDecl)
        ctype = cpp_struct_t;
    else if (kind == CXCursor_UnionDecl)
        ctype = cpp_union_t;
    else
        assert(false);

    auto name = detail::parse_name(cur);
    bool is_final;
    auto definition = parse_class(tu, cur, name, is_final);
    if (definition)
        class_ = cpp_ptr<cpp_class>(new cpp_class(std::move(scope), std::move(name), detail::parse_comment(cur),
                                    clang_getCursorType(cur), ctype, is_final));
}

void cpp_class::parser::add_entity(cpp_entity_ptr ptr)
{
    assert(class_);
    class_->add_entity(std::move(ptr));
}

cpp_name cpp_class::parser::scope_name()
{
    return class_ ? class_->get_name() : "";
}

cpp_entity_ptr cpp_class::parser::finish(const standardese::parser &par)
{
    if (class_)
        par.register_type(*class_);
    return std::move(class_);
}