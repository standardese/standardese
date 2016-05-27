// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_namespace.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/cpp_cursor.hpp>
#include <standardese/parser.hpp>

using namespace standardese;

namespace
{
    bool is_inline_namespace(translation_unit &tu, CXCursor cur)
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

cpp_namespace::parser::parser(translation_unit &tu, const cpp_name &scope, cpp_cursor cur)
: ns_(new cpp_namespace(scope, detail::parse_name(cur), detail::parse_comment(cur)))
{
    assert(clang_getCursorKind(cur) == CXCursor_Namespace);
    if (is_inline_namespace(tu, cur))
        ns_->inline_ = true;

}

cpp_entity_ptr cpp_namespace::parser::finish(const standardese::parser &par)
{
    par.register_namespace(*ns_);
    return std::move(ns_);
}

namespace
{
    cpp_name parse_target_scope(CXCursor cur)
    {
        cpp_name scope;
        detail::visit_children(cur, [&](CXCursor cur, CXCursor)
        {
            assert(clang_isReference(clang_getCursorKind(cur)));

            auto ref_cursor = clang_getCursorReferenced(cur);
            scope = detail::parse_scope(ref_cursor);

            return CXChildVisit_Break;
        });

        return scope;
    }

    cpp_name parse_target(detail::token_stream &stream)
    {
        cpp_name target;
        while (!stream.done())
            if (stream.peek().get_value() != "\n")
                target += stream.get().get_value().c_str();
            else
                stream.get();
        return target;
    }
}

cpp_ptr<cpp_namespace_alias> cpp_namespace_alias::parse(translation_unit &tu, cpp_name scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_NamespaceAlias);

    auto name = detail::parse_name(cur);
    auto target_scope = parse_target_scope(cur);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);
    auto location = source_location(clang_getCursorLocation(cur), name);

    detail::skip(stream, location, {"namespace", name.c_str(), "="});
    auto target = parse_target(stream);

    auto result = detail::make_ptr<cpp_namespace_alias>(std::move(scope), std::move(name),
                                                 detail::parse_comment(cur), target);
    result->unique_ = target_scope.empty() ? std::move(target) : target_scope + "::" + target;
    return result;
}

cpp_ptr<cpp_using_directive> cpp_using_directive::parse(translation_unit &tu, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_UsingDirective);

    auto target_scope = parse_target_scope(cur);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);
    auto location = source_location(clang_getCursorLocation(cur), "using namespace");

    detail::skip(stream, location, {"using", "namespace"});
    auto target = parse_target(stream);

    return detail::make_ptr<cpp_using_directive>(std::move(target_scope), std::move(target), detail::parse_comment(cur));
}

cpp_ptr<cpp_using_declaration> cpp_using_declaration::parse(translation_unit &tu, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_UsingDeclaration);

    auto target_scope = parse_target_scope(cur);

    detail::tokenizer tokenizer(tu, cur);
    auto stream = detail::make_stream(tokenizer);
    auto location = source_location(clang_getCursorLocation(cur), "using");

    detail::skip(stream, location, {"using"});
    auto target = parse_target(stream);

    return detail::make_ptr<cpp_using_declaration>(std::move(target_scope), std::move(target), detail::parse_comment(cur));
}
