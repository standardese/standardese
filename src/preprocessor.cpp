// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/preprocessor.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'sprintf' : format string '%ld' requires an argument of type 'long', but variadic argument 1 has type 'size_t'
#pragma warning(disable : 4477)
#endif

#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include <boost/wave.hpp>
#include <boost/version.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if (BOOST_VERSION / 100000) != 1
#error "require Boost 1.x"
#endif

#if ((BOOST_VERSION / 100) % 1000) < 55
#warning "Boost less than 1.55 isn't tested"
#endif

#include <standardese/config.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;
namespace bw = boost::wave;
namespace fs = boost::filesystem;

namespace
{
    using input_iterator = std::string::const_iterator;
    using token_iterator = bw::cpplexer::lex_iterator<bw::cpplexer::lex_token<>>;

    class policy : public bw::context_policies::default_preprocessing_hooks
    {
    public:
        policy(const preprocessor& pre, cpp_file& file, std::string& preprocessed)
        : pre_(&pre), file_(&file), preprocessed_(&preprocessed)
        {
        }

        template <class ContextT, class TokenT>
        const TokenT& generated_token(const ContextT& ctx, const TokenT& token)
        {
            if (token.is_valid() && ctx.get_iteration_depth() == 0)
                // only add main file tokens
                *preprocessed_ += token.get_value().c_str();
            return token;
        }

        template <class ContextT, class ContainerT>
        bool found_warning_directive(const ContextT&, const ContainerT&)
        {
            // ignore warnings
            return true;
        }

        template <class ContextT, class TokenT, class ContainerT>
        bool evaluated_conditional_expression(const ContextT& ctx, const TokenT& directive,
                                              const ContainerT& expression, bool value)
        {
            if (found_guard_ || ctx.get_iteration_depth() != 0)
                // already found, not in main file
                return false;
            else if (value || directive.get_value() != "#ifndef")
                // invalid directive
                return false;
            else if (expression.size() > 1)
                // more than one token in the expression
                return false;

            // remember include guard
            include_guard_ = expression.begin()->get_value().c_str();
            ifndef_line_   = expression.begin()->get_position().get_line();
            return false;
        }

        template <class ContextT>
        bool found_include_directive(const ContextT& ctx, std::string file_name, bool include_next)
        {
            bool is_system;
            file_name = get_include_kind(file_name, is_system);

            auto use = use_include(ctx, file_name, is_system, include_next);
            if (use && ctx.get_iteration_depth() == 0)
                file_->add_entity(
                    cpp_inclusion_directive::make(*file_, file_name,
                                                  is_system ? cpp_inclusion_directive::system :
                                                              cpp_inclusion_directive::local,
                                                  0));

            // write include so that libclang can use it
            *preprocessed_ += '#';
            *preprocessed_ += include_next ? "include_next" : "include";
            *preprocessed_ += is_system ? '<' : '"';
            *preprocessed_ += file_name;
            *preprocessed_ += is_system ? '>' : '"';
            *preprocessed_ += '\n';

            return !use; // only parse if used
        }

        template <class ContextT, class ParametersT, class DefinitionT>
        void defined_macro(const ContextT& ctx, const bw::cpplexer::lex_token<>& name,
                           bool is_function_like, const ParametersT& parameters,
                           const DefinitionT& definition, bool is_predefined)
        {
            if (is_predefined || ctx.get_iteration_depth() != 0)
                // not in the main file
                return;
            else if (!found_guard_ && !include_guard_.empty())
            {
                // include guard not found, but encountered directive
                if (name.get_position().get_line() == ifndef_line_ + 1
                    && name.get_value() == include_guard_.c_str())
                {
                    // this is in the next line and has the same macro name
                    // treat it as include guard
                    // need to write two newlines for the ifndef and the macro
                    *preprocessed_ += "\n\n";
                    found_guard_ = true;
                    return;
                }
                else
                {
                    // reset state, not include guard
                    include_guard_.clear();
                    ifndef_line_ = 0;
                }
            }

            std::string str_name = name.get_value().c_str();

            std::string str_params;
            if (is_function_like)
            {
                str_params += '(';
                auto needs_comma = false;
                for (auto& token : parameters)
                {
                    if (needs_comma)
                        str_params += ", ";
                    else
                        needs_comma = true;
                    str_params += token.get_value().c_str();
                }
                str_params += ')';
            }

            std::string str_def;
            for (auto& token : definition)
                str_def += token.get_value().c_str();

            // also write macro, so that it can be documented
            *preprocessed_ += "#define " + str_name + str_params + ' ' + str_def + '\n';

            file_->add_entity(cpp_macro_definition::make(*file_, std::move(str_name),
                                                         std::move(str_params), std::move(str_def),
                                                         unsigned(name.get_position().get_line())));
        }

        template <class ContextT>
        void undefined_macro(const ContextT& ctx, const bw::cpplexer::lex_token<>& name)
        {
            if (ctx.get_iteration_depth() != 0)
                // not in the main file
                return;

            auto prev = static_cast<cpp_entity*>(nullptr);
            for (auto& entity : *file_)
            {
                if (entity.get_name() == name.get_value().c_str())
                    break;
                prev = &entity;
            }

            file_->remove_entity_after(prev);
        }

    private:
        std::string get_include_kind(std::string file_name, bool& is_system)
        {
            assert(file_name[0] == '<' || file_name[0] == '"');
            is_system = file_name[0] == '<';

            file_name.erase(file_name.begin());
            file_name.pop_back();
            return file_name;
        }

        template <class ContextT>
        bool use_include(const ContextT& ctx, std::string file_name, bool is_system,
                         bool include_next)
        {
            if (include_next)
                return false;

            std::string dir;
            if (!ctx.find_include_file(file_name, dir, is_system, nullptr))
                return false;

            auto     dir_path = fs::path(dir);
            fs::path cur_path;
            for (auto part : dir_path)
            {
                cur_path /= part;
                if (pre_->is_preprocess_directory(cur_path.generic_string()))
                    return true;
            }
            return false;
        }

        const preprocessor* pre_;
        cpp_file*           file_;
        std::string*        preprocessed_;

        unsigned header_count_;

        std::string include_guard_;
        unsigned    ifndef_line_ = 0;
        bool        found_guard_ = false;
    };

    using context = bw::context<input_iterator, token_iterator,
                                bw::iteration_context_policies::load_file_to_string, policy>;

    void setup_context(context& cont, const compile_config& c)
    {
        // set language to C++11 preprecessor
        // inserts additional whitespace to separate tokens
        // do not emits line directives
        // preserve comments
        auto lang = bw::support_cpp | bw::support_option_variadics | bw::support_option_long_long
                    | bw::support_option_insert_whitespace
                    | ~bw::support_option_emit_line_directives
                    | bw::support_option_preserve_comments;
        cont.set_language(bw::language_support(lang));

        // add macros and include paths
        for (auto iter = c.begin(); iter != c.end(); ++iter)
        {
            if (*iter == "-D")
            {
                ++iter;
                cont.add_macro_definition(iter->c_str());
            }
            else if (*iter == "-U")
            {
                ++iter;
                cont.remove_macro_definition(iter->c_str());
            }
            else if (*iter == "-I")
            {
                ++iter;
                cont.add_sysinclude_path(iter->c_str());
            }
            else if (iter->c_str()[0] == '-')
            {
                if (iter->c_str()[1] == 'D')
                    cont.add_macro_definition(&(iter->c_str()[2]));
                else if (iter->c_str()[1] == 'U')
                    cont.remove_macro_definition(&(iter->c_str()[2]));
                else if (iter->c_str()[1] == 'I')
                    cont.add_include_path(&(iter->c_str()[2]));
            }
        }
    }
}

std::string preprocessor::preprocess(const compile_config& c, const char* full_path,
                                     const std::string& source, cpp_file& file) const
{
    std::string preprocessed;

    context cont(source.begin(), source.end(), full_path, policy(*this, file, preprocessed));
    setup_context(cont, c);
    for (auto iter = cont.begin(); iter != cont.end(); ++iter)
        // do nothing, policy does it all
        ;

    return preprocessed;
}

void preprocessor::add_preprocess_directory(std::string dir)
{
    auto path = fs::system_complete(dir);
    preprocess_dirs_.insert(path.normalize().generic_string());
}

bool preprocessor::is_preprocess_directory(const std::string& dir) const STANDARDESE_NOEXCEPT
{
    auto path = fs::system_complete(dir);
    return preprocess_dirs_.count(path.normalize().generic_string()) != 0;
}
