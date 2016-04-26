// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/translation_unit.hpp>

#include <iostream>
#include <vector>

#include <standardese/cpp_cursor.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/parser.hpp>
#include <standardese/string.hpp>

using namespace standardese;

cpp_file::cpp_file(const char *name)
: cpp_entity(name, "")
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
        scope_name += parser->scope_name() + "::";
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
        if (stack_.size() > 1u && clang_equalCursors(parent, stack_.back().parent))
        {
            // current parent is equal to top parent
            // and we aren't removing the file
            auto ptr = stack_.back().parser->finish(par);
            stack_.pop_back();
            stack_.back().parser->add_entity(std::move(ptr));
            return true;
        }
        else
        {
            // current parent isn't equal to top parent
            // need to search for any previous containers to see if their parent matches
            // this can happen when multiple scopes are left at once
            // no need to check for std::prev(stack_.end()), done in the fast check
            // also prevents running the loop when stack containers only the file
            for (auto iter = stack_.begin(); iter != std::prev(stack_.end()); ++iter)
                if (clang_equalCursors(iter->parent, parent))
                {
                    // we found a previous one, erase all after that
                    for (auto cur = stack_.end(); cur != std::next(iter); --cur)
                    {
                        auto e = std::prev(cur)->parser->finish(par);
                        std::prev(cur, 2)->parser->add_entity(std::move(e));
                    }
                    stack_.erase(std::next(iter), stack_.end());
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

    auto kind = clang_getCursorKind(cur);
    switch (kind)
    {
        case CXCursor_Namespace:
            stack.push_container(cpp_ptr<cpp_entity_parser>(new cpp_namespace::parser(stack.get_scope_name(), cur)),
                                 parent);
            return CXChildVisit_Recurse;

        default:
        {
            string str(clang_getCursorKindSpelling(kind));
            std::cerr << "Unknown cursor kind: " << str << '\n';
            break;
        }
    }

    return CXChildVisit_Continue;
}
