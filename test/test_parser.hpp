// Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_TEST_PARSER_HPP_INCLUDED
#define STANDARDESE_TEST_PARSER_HPP_INCLUDED

#include <fstream>

#include "../external/catch/single_include/catch2/catch.hpp"

#include <cppast/libclang_parser.hpp>
#include <cppast/visitor.hpp>

#include <standardese/comment.hpp>
#include <standardese/doc_entity.hpp>

#include "test_logger.hpp"
#include "util/indent.hpp"

inline std::unique_ptr<cppast::cpp_file> parse_file(const cppast::cpp_entity_index& idx,
                                                    const char* name, const char* content)
{
    static cppast::libclang_compile_config config;
    static cppast::libclang_parser         parser(test_logger());
    config.set_flags(cppast::cpp_standard::cpp_latest);

    std::ofstream file(name);
    file << standardese::test::util::unindent(content);
    file.close();

    return parser.parse(idx, name, config);
}

inline const cppast::cpp_entity& get_named_entity(const cppast::cpp_file& file, const char* name)
{
    const cppast::cpp_entity* result = nullptr;
    cppast::visit(file, [&](const cppast::cpp_entity& e, const cppast::visitor_info&) {
        if (e.name() == name)
        {
            result = &e;
            return false;
        }
        else
            return true;
    });
    REQUIRE(result);
    return *result;
}

inline const standardese::doc_entity& get_named_entity(const standardese::doc_cpp_file& file,
                                                       const char*                      name)
{
    auto& cpp_entity = get_named_entity(file.file(), name);
    REQUIRE(cpp_entity.user_data());
    return *static_cast<const standardese::doc_entity*>(cpp_entity.user_data());
}

inline standardese::comment_registry parse_comments(const cppast::cpp_file& file)
{
    standardese::file_comment_parser parser(test_logger());
    parser.parse(type_safe::ref(file));
    return parser.finish();
}

inline std::unique_ptr<standardese::doc_cpp_file> build_doc_entities(
    const standardese::comment_registry& comments, const
    cppast::cpp_entity_index& index, std::unique_ptr<cppast::cpp_file> file,
    const standardese::entity_blacklist& blacklist = {}, bool hide_uncommented = false)
{
    auto name = file->name();
    standardese::exclude_entities(comments, index, blacklist, hide_uncommented, *file);
    return standardese::build_doc_entities(type_safe::ref(comments), index, std::move(file),
                                           std::move(name));
}

inline std::unique_ptr<standardese::doc_cpp_file> build_doc_entities(
    standardese::comment_registry& comments, const cppast::cpp_entity_index& index,
    const char* name, const char* source, const standardese::entity_blacklist&
    blacklist = {}, bool hide_uncommented = false)
{
    auto file = parse_file(index, name, source);
    comments.merge(parse_comments(*file));
    return build_doc_entities(comments, index, std::move(file), blacklist, hide_uncommented);
}

#endif // STANDARDESE_TEST_PARSER_HPP_INCLUDED
