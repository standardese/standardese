// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/detail/preprocessor.hpp>

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

using namespace standardese;
namespace bw = boost::wave;

namespace
{
    using input_iterator = std::string::const_iterator;
    using token_iterator = bw::cpplexer::lex_iterator<bw::cpplexer::lex_token<>>;

    using context = bw::context<input_iterator, token_iterator>;

    void setup_context(context& cont, const compile_config& c)
    {
        // set language to C++11 preprecessor
        // inserts additional whitespace to separate tokens
        // emits line directives
        // preserve comments
        auto lang = bw::support_cpp | bw::support_option_variadics | bw::support_option_long_long
                    | bw::support_option_insert_whitespace | bw::support_option_emit_line_directives
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

std::string detail::preprocess(const compile_config& c, const char* full_path,
                               const std::string& source)
{
    context cont(source.begin(), source.end(), full_path);
    setup_context(cont, c);

    std::string preprocessed;
    for (auto& token : cont)
        preprocessed += token.get_value().c_str();
    return preprocessed;
}
