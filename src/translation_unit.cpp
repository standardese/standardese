// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/translation_unit.hpp>

#include <vector>

#include <standardese/detail/tokenizer.hpp>
#include <standardese/cpp_class.hpp>
#include <standardese/cpp_cursor.hpp>
#include <standardese/cpp_enum.hpp>
#include <standardese/cpp_function.hpp>
#include <standardese/cpp_namespace.hpp>
#include <standardese/cpp_preprocessor.hpp>
#include <standardese/cpp_template.hpp>
#include <standardese/cpp_type.hpp>
#include <standardese/cpp_variable.hpp>
#include <standardese/error.hpp>
#include <standardese/parser.hpp>
#include <standardese/string.hpp>

using namespace standardese;

cpp_file::cpp_file(const char *name)
: cpp_entity(file_t, "", name, "")
{}

struct detail::context_impl::impl
{
    detail::context context;

    impl(const std::string &path)
    : context(path.begin(), path.end(), path.c_str()) {} // initialize with empty range
};

detail::context_impl::context_impl(context_impl &&other) STANDARDESE_NOEXCEPT
: pimpl(std::move(other.pimpl)) {}

detail::context_impl::~context_impl() STANDARDESE_NOEXCEPT {}

detail::context_impl& detail::context_impl::operator=(context_impl &&other) STANDARDESE_NOEXCEPT
{
    pimpl = std::move(other.pimpl);
    return *this;
}

namespace standardese { namespace detail
{
    // get context impl from translation unit
    // needed because this must be a friend of translation_unit
    // and context cannot be forward declared
    context_impl& get_context_impl(translation_unit &tu)
    {
        return tu.impl_;
    }

    // hidden function to get the context from the impl
    context& get_preprocessing_context(context_impl &impl)
    {
        return impl.pimpl->context;
    }
}} // namespace standardese::detail

translation_unit::translation_unit(const parser &par, CXTranslationUnit tu, const char *path)
: tu_(tu), path_(path),
  parser_(&par)
{
    using namespace boost::wave;

    impl_.pimpl = std::unique_ptr<detail::context_impl::impl>(new detail::context_impl::impl(path_));

    auto lang = support_cpp | support_option_variadics | support_option_long_long
                | support_option_insert_whitespace
                | support_option_single_line;
    impl_.pimpl->context.set_language(language_support(lang));
}

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

cpp_file& translation_unit::build_ast()
{
    cpp_ptr<cpp_file> result(new cpp_file(get_path()));

    scope_stack stack(result.get(), clang_getTranslationUnitCursor(tu_.get()));
    visit([&](CXCursor cur, CXCursor parent) {return this->parse_visit(stack, cur, parent);});
    stack.pop_if_needed(clang_getTranslationUnitCursor(tu_.get()), *parser_);

    auto& ref = *result;
    parser_->register_file(std::move(result));
    return ref;
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

namespace
{
    void add_macro_definition(translation_unit &tu, detail::context &context, cpp_cursor definition)
    {
        auto macro = cpp_macro_definition::parse(tu, definition);

        auto string = macro->get_name() + macro->get_argument_string()
                      + "=" + macro->get_replacement();
        context.add_macro_definition(std::move(string));
    }
}

CXChildVisitResult translation_unit::parse_visit(scope_stack &stack, CXCursor cur, CXCursor parent) try
{
    stack.pop_if_needed(parent, *parser_);

    auto scope = stack.get_scope_name();

    auto kind = clang_getCursorKind(cur);
    if (kind != CXCursor_CXXBaseSpecifier
        && (clang_isReference(kind) || clang_isExpression(kind)))
        // ignore those
        return CXChildVisit_Continue;

    switch (kind)
    {
        case CXCursor_InclusionDirective:
            stack.add_entity(cpp_inclusion_directive::parse(*this, cur));
            return CXChildVisit_Continue;
        case CXCursor_MacroDefinition:
            stack.add_entity(cpp_macro_definition::parse(*this, cur));
            return CXChildVisit_Continue;
        case CXCursor_MacroExpansion:
            add_macro_definition(*this, impl_.pimpl->context, clang_getCursorReferenced(cur));
            return CXChildVisit_Continue;

        case CXCursor_Namespace:
            stack.push_container(detail::make_ptr<cpp_namespace::parser>(*this, scope, cur), parent);
            return CXChildVisit_Recurse;
        case CXCursor_NamespaceAlias:
            stack.add_entity(cpp_namespace_alias::parse(*this, scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_UsingDirective:
            stack.add_entity(cpp_using_directive::parse(*this, cur));
            return CXChildVisit_Continue;
        case CXCursor_UsingDeclaration:
            stack.add_entity(cpp_using_declaration::parse(*this, cur));
            return CXChildVisit_Continue;

        case CXCursor_TypedefDecl:
        case CXCursor_TypeAliasDecl:
            stack.add_entity(cpp_type_alias::parse(*this, scope, cur));
            return CXChildVisit_Continue;

        case CXCursor_EnumDecl:
            stack.push_container(detail::make_ptr<cpp_enum::parser>(*this, scope, cur), parent);
            return CXChildVisit_Recurse;
        case CXCursor_EnumConstantDecl:
            stack.add_entity(cpp_enum_value::parse(*this, scope, cur));
            return CXChildVisit_Continue;

        case CXCursor_VarDecl:
            stack.add_entity(cpp_variable::parse(*this, scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_FieldDecl:
            stack.add_entity(cpp_member_variable::parse(*this, scope, cur));
            return CXChildVisit_Continue;

        case CXCursor_FunctionDecl:
            if (is_full_specialization(*this, cur))
                stack.add_entity(cpp_function_template_specialization::parse(*this, scope, cur));
            else
                stack.add_entity(cpp_function::parse(*this, scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_CXXMethod:
            if (is_full_specialization(*this, cur))
                stack.add_entity(cpp_function_template_specialization::parse(*this, scope, cur));
            else
                stack.add_entity(cpp_member_function::parse(*this, scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_ConversionFunction:
            if (is_full_specialization(*this, cur))
                stack.add_entity(cpp_function_template_specialization::parse(*this, scope, cur));
            else
                stack.add_entity(cpp_conversion_op::parse(*this, scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_Constructor:
            if (is_full_specialization(*this, cur))
                stack.add_entity(cpp_function_template_specialization::parse(*this, scope, cur));
            else
                stack.add_entity(cpp_constructor::parse(*this, scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_Destructor:
            stack.add_entity(cpp_destructor::parse(*this, scope, cur));
            return CXChildVisit_Continue;

        case CXCursor_FunctionTemplate:
            stack.add_entity(cpp_function_template::parse(*this, scope, cur));
            return CXChildVisit_Continue;

        case CXCursor_ClassDecl:
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
            if (is_full_specialization(*this, cur))
                stack.push_container(detail::make_ptr<cpp_class_template_full_specialization::parser>
                                                       (*this, scope, cur), parent);
            else
                stack.push_container(detail::make_ptr<cpp_class::parser>(*this, scope, cur), parent);
            return CXChildVisit_Recurse;
        case CXCursor_ClassTemplate:
            stack.push_container(detail::make_ptr<cpp_class_template::parser>(*this, scope, cur), parent);
            return CXChildVisit_Recurse;
        case CXCursor_ClassTemplatePartialSpecialization:
            stack.push_container(detail::make_ptr<cpp_class_template_partial_specialization::parser>
                                                    (*this, scope, cur), parent);
            return CXChildVisit_Recurse;
        case CXCursor_CXXBaseSpecifier:
            stack.add_entity(cpp_base_class::parse(*this, scope, cur));
            return CXChildVisit_Continue;
        case CXCursor_CXXAccessSpecifier:
            stack.add_entity(cpp_access_specifier::parse(*this, cur));
            return CXChildVisit_Continue;

        // ignored, because handled elsewhere
        case CXCursor_TemplateTypeParameter:
        case CXCursor_TemplateTemplateParameter:
        case CXCursor_NonTypeTemplateParameter:
        case CXCursor_CXXFinalAttr:
        case CXCursor_ParmDecl:
            return CXChildVisit_Continue;

        default:
        {
            string str(clang_getCursorKindSpelling(kind));
            parser_->get_logger()->warn("Unknown cursor kind \'{}\'", str);
            break;
        }
    }

    return CXChildVisit_Continue;
}
catch (parse_error &ex)
{
    parser_->get_logger()->error("when parsing {} ({}:{}): {}",
                                 ex.get_location().entity_name, ex.get_location().file_name, ex.get_location().line,
                                 ex.what());
    return CXChildVisit_Continue;
}
