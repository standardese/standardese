// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_type.hpp>

#include <cassert>

#include <standardese/detail/parse_utils.hpp>
#include <standardese/detail/tokenizer.hpp>
#include <standardese/parser.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

namespace
{
    cpp_type_ref parse_alias_target(translation_unit &tu, cpp_cursor cur, const cpp_name &name)
    {
        auto type = clang_getTypedefDeclUnderlyingType(cur);
        cpp_name target_name;

        detail::tokenizer tokenizer(tu, cur);
        auto stream = detail::make_stream(tokenizer);
        source_location location(clang_getCursorLocation(cur), name);

        if (clang_getCursorKind(cur) == CXCursor_TypeAliasDecl)
        {
            skip(stream, location, {"using", name.c_str(), "="});

            while (stream.peek().get_value() != ";")
            {
                detail::skip_attribute(stream, location);
                target_name += stream.get().get_value().c_str();
            }
        }
        else
        {
            assert(clang_getCursorKind(cur) == CXCursor_TypedefDecl);

            skip(stream, location, {"typedef"});

            while (stream.peek().get_value() != ";")
            {
                detail::skip_attribute(stream, location);
                auto& val = stream.peek().get_value();
                if (val != name.c_str())
                    target_name += val.c_str();

                stream.bump();
            }
        }

        while (std::isspace(target_name.back()))
            target_name.pop_back();

        return {type, target_name};
    }
}

cpp_name cpp_type_ref::get_full_name() const
{
    auto name = detail::parse_name(type_);

    // when the name of the class is mentioned in a partial specialization
    // clang appends the template parameters with an internal name at the end
    // erase that part
    if (name.find("-parameter") != cpp_name::npos)
        detail::clean_name(name);

    return name;
}

cpp_ptr<cpp_type_alias> cpp_type_alias::parse(translation_unit &tu, const cpp_name &scope, cpp_cursor cur)
{
    assert(clang_getCursorKind(cur) == CXCursor_TypedefDecl
           || clang_getCursorKind(cur) == CXCursor_TypeAliasDecl);

    auto name = detail::parse_name(cur);
    auto target = parse_alias_target(tu, cur, name);
    auto result = detail::make_ptr<cpp_type_alias>(scope, std::move(name), detail::parse_comment(cur),
                                                   clang_getCursorType(cur), target);

    tu.get_parser().register_type(*result);

    return result;
}