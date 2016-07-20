// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_namespace.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

cpp_ptr<cpp_language_linkage> cpp_language_linkage::parse(translation_unit& tu, cpp_cursor cur,
                                                          const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_UnexposedDecl);

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);

    detail::skip(stream, cur, {"extern"});
    auto   str = stream.get().get_value();
    string name(str.c_str() + 1, str.size() - 2); // cut quotes

    auto result = detail::make_cpp_ptr<cpp_language_linkage>(cur, parent, std::move(name));
    result->set_comment(tu);
    return result;
}

namespace
{
    bool is_inline_namespace(translation_unit& tu, cpp_cursor cur)
    {
        detail::tokenizer tokenizer(tu, cur);
        for (auto val : tokenizer)
            if (val.get_value() == "inline")
                return true;
            else if (val.get_value() == "namespace")
                break;
        return false;
    }
}

cpp_ptr<cpp_namespace> cpp_namespace::parse(translation_unit& tu, cpp_cursor cur,
                                            const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_Namespace);

    auto is_inline = is_inline_namespace(tu, cur);
    auto result    = detail::make_cpp_ptr<cpp_namespace>(cur, parent, is_inline);
    result->set_comment(tu);
    return result;
}

namespace
{
    cpp_cursor parse_target_cursor(CXCursor cur)
    {
        cpp_cursor result;
        detail::visit_children(cur, [&](CXCursor cur, CXCursor) {
            if (clang_isReference(clang_getCursorKind(cur)))
                result = cur;

            return CXChildVisit_Recurse;
        });

        return result;
    }

    cpp_name parse_target(detail::token_stream& stream)
    {
        std::string target;
        while (!stream.done())
            if (stream.peek().get_value() != ";")
                target += stream.get().get_value().c_str();
            else
                stream.get();
        detail::erase_trailing_ws(target);
        return target;
    }
}

cpp_ptr<cpp_namespace_alias> cpp_namespace_alias::parse(translation_unit& tu, cpp_cursor cur,
                                                        const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_NamespaceAlias);

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);

    auto name = detail::parse_name(cur);
    detail::skip(stream, cur, {"namespace", name.c_str(), "="});

    auto target        = parse_target(stream);
    auto target_cursor = parse_target_cursor(cur);

    auto result =
        detail::make_cpp_ptr<cpp_namespace_alias>(cur, parent,
                                                  cpp_namespace_ref(target_cursor, target));
    result->set_comment(tu);
    return result;
}

cpp_ptr<cpp_using_directive> cpp_using_directive::parse(translation_unit& tu, cpp_cursor cur,
                                                        const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_UsingDirective);

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);

    detail::skip(stream, cur, {"using", "namespace"});

    auto target        = parse_target(stream);
    auto target_cursor = parse_target_cursor(cur);

    auto result =
        detail::make_cpp_ptr<cpp_using_directive>(cur, parent,
                                                  cpp_namespace_ref(target_cursor, target));
    result->set_comment(tu);
    return result;
}

cpp_ptr<cpp_using_declaration> cpp_using_declaration::parse(translation_unit& tu, cpp_cursor cur,
                                                            const cpp_entity& parent)
{
    assert(clang_getCursorKind(cur) == CXCursor_UsingDeclaration);

    detail::tokenizer tokenizer(tu, cur);
    auto              stream = detail::make_stream(tokenizer);

    detail::skip(stream, cur, {"using"});

    auto target        = parse_target(stream);
    auto target_cursor = parse_target_cursor(cur);

    auto result =
        detail::make_cpp_ptr<cpp_using_declaration>(cur, parent,
                                                    cpp_entity_ref(target_cursor, target));
    result->set_comment(tu);
    return result;
}
