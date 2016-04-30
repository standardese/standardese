// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/translation_unit.hpp>

#include <iostream>
#include <vector>

#include <standardese/cpp_class.hpp>
#include <standardese/cpp_cursor.hpp>
#include <standardese/cpp_enum.hpp>
#include <standardese/cpp_function.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/cpp_type.hpp>
#include <standardese/cpp_variable.hpp>
#include <standardese/parser.hpp>
#include <standardese/string.hpp>

using namespace standardese;

cpp_file::cpp_file(const char *name)
: cpp_entity("", name, "")
{}

translation_unit::translation_unit(const parser &par, CXTranslationUnit tu, const char *path)
: tu_(tu), path_(path), parser_(&par)
{}

class translation_unit::scope_stack
{
public:
    // give it the file
    // this is always the first element and will never be erased
    scope_stack(cpp_file *f, CXCursor parent)
    {
        struct cpp_file_parser : cpp_entity_parser
        {
            cpp_file *f;

            cpp_file_parser(cpp_file *f)
            : f(f) {}

            void add_entity(cpp_entity_ptr e) override
            {
                f->add_entity(std::move(e));
            }

            cpp_entity_ptr finish(const parser &) override
            {
                return nullptr;
            }
        };

        stack_.emplace_back(cpp_ptr<cpp_entity_parser>(new cpp_file_parser{f}), "", parent);
    }

    const cpp_name& get_scope_name() const STANDARDESE_NOEXCEPT
    {
        return stack_.back().scope_name;
    }

    // pushes a new container
    void push_container(cpp_ptr<cpp_entity_parser> parser, CXCursor parent)
    {
        auto scope_name = stack_.back().scope_name;
        if (!scope_name.empty())
            scope_name += "::";
        scope_name += parser->scope_name();
        stack_.emplace_back(std::move(parser), scope_name, parent);
    }

    // adds a non-container entity to the current container
    void add_entity(cpp_entity_ptr e)
    {
        auto& top  = stack_.back();
        top.parser->add_entity(std::move(e));
    }

    // pops a container if needed
    // needs to be called in each visit
    bool pop_if_needed(CXCursor parent, const parser &par)
    {
        for (auto iter = std::next(stack_.begin()); iter != stack_.end(); ++iter)
        {
            if (clang_equalCursors(iter->parent, parent))
            {
                auto dist = stack_.end() - iter;
                for (auto i = 0; i != dist; ++i)
                {
                    auto e = stack_.back().parser->finish(par);
                    stack_.pop_back();
                    assert(!stack_.empty());
                    stack_.back().parser->add_entity(std::move(e));
                }

                return true;
            }
        }

        return false;
    }

private:
    struct container
    {
        cpp_ptr<cpp_entity_parser> parser;
        cpp_name scope_name;
        CXCursor parent;

        container(cpp_ptr<cpp_entity_parser> par, cpp_name scope_name, CXCursor parent)
        : parser(std::move(par)), scope_name(std::move(scope_name)), parent(parent)
        {}
    };

    std::vector<container> stack_;
};

cpp_ptr<cpp_file> translation_unit::parse() const
{
    cpp_ptr<cpp_file> result(new cpp_file(get_path()));

    scope_stack stack(result.get(), clang_getTranslationUnitCursor(tu_.get()));
    visit([&](CXCursor cur, CXCursor parent) {return this->parse_visit(stack, cur, parent);});
    stack.pop_if_needed(clang_getTranslationUnitCursor(tu_.get()), *parser_);

    parser_->register_file(*result);
    return result;
}

CXFile translation_unit::get_cxfile() const STANDARDESE_NOEXCEPT
{
    auto file = clang_getFile(tu_.get(), get_path());
    detail::validate(file);
    return file;
}

void translation_unit::deleter::operator()(CXTranslationUnit tu) const STANDARDESE_NOEXCEPT
{
    clang_disposeTranslationUnit(tu);
}

CXChildVisitResult translation_unit::parse_visit(scope_stack &stack, CXCursor cur, CXCursor parent) const
{
    stack.pop_if_needed(parent, *parser_);

    auto scope = stack.get_scope_name();

    auto kind = clang_getCursorKind(cur);
    switch (kind)
    {
        case CXCursor_Namespace:
            stack.push_container(detail::make_ptr<cpp_namespace::parser>(scope, cur), parent);
            return CXChildVisit_Recurse;
        case CXCursor_NamespaceAlias:
            stack.add_entity(cpp_namespace_alias::parse(scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_UsingDirective:
            stack.add_entity(cpp_using_directive::parse(cur));
            return CXChildVisit_Continue;

        case CXCursor_TypedefDecl:
        case CXCursor_TypeAliasDecl:
            stack.add_entity(cpp_type_alias::parse(*parser_, scope, cur));
            return CXChildVisit_Continue;

        case CXCursor_EnumDecl:
            stack.push_container(detail::make_ptr<cpp_enum::parser>(scope, cur), parent);
            return CXChildVisit_Recurse;
        case CXCursor_EnumConstantDecl:
            stack.add_entity(cpp_enum_value::parse(scope, cur));
            return CXChildVisit_Continue;

        case CXCursor_VarDecl:
            stack.add_entity(cpp_variable::parse(scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_FieldDecl:
            stack.add_entity(cpp_member_variable::parse(scope, cur));
            return CXChildVisit_Continue;

        case CXCursor_FunctionDecl:
            stack.add_entity(cpp_function::parse(scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_CXXMethod:
            stack.add_entity(cpp_member_function::parse(scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_ConversionFunction:
            stack.add_entity(cpp_conversion_op::parse(scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_Constructor:
            stack.add_entity(cpp_constructor::parse(scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_Destructor:
            stack.add_entity(cpp_destructor::parse(scope, cur));
            return CXChildVisit_Continue;

        case CXCursor_ClassDecl:
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
            stack.push_container(detail::make_ptr<cpp_class::parser>(scope, cur), parent);
            return CXChildVisit_Recurse;
        case CXCursor_CXXBaseSpecifier:
            stack.add_entity(cpp_base_class::parse(scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_CXXAccessSpecifier:
            stack.add_entity(cpp_access_specifier::parse(cur));
            return CXChildVisit_Continue;

        default:
        {
            string str(clang_getCursorKindSpelling(kind));
            std::cerr << "Unknown cursor kind: " << str << '\n';
            break;
        }
    }

    return CXChildVisit_Continue;
}
