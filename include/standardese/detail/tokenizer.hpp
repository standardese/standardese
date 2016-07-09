// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_TOKENIZER_HPP_INCLUDED
#define STANDARDESE_DETAIL_TOKENIZER_HPP_INCLUDED

#include <initializer_list>
#include <string>

#ifdef _MSC_VER
#pragma warning(push)
// 'sprintf' : format string '%ld' requires an argument of type 'long', but variadic argument 1 has type 'size_t'
#pragma warning(disable : 4477)
#endif

#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include <boost/wave.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <standardese/detail/sequence_stream.hpp>
#include <standardese/error.hpp>
#include <standardese/string.hpp>
#include <standardese/cpp_cursor.hpp>

namespace standardese
{
    struct cpp_cursor;
    class translation_unit;
} // namespace standardese

namespace standardese
{
    namespace detail
    {
        using token_iterator =
            boost::wave::cpplexer::lex_iterator<boost::wave::cpplexer::lex_token<>>;

        // inherit from it to allow forward declaration
        struct context : public boost::wave::context<std::string::const_iterator, token_iterator>
        {
            context(const std::string& path)
            : boost::wave::context<std::string::const_iterator, token_iterator>(path.end(),
                                                                                path.end(),
                                                                                path.c_str())
            {
            }
        };

        using token_stream = sequence_stream<context::iterator_type>;

        // skips all whitespace
        void skip_whitespace(token_stream& stream);

        // skips "value" and asserts that it is actually there
        void skip(token_stream& stream, const cpp_cursor& cur, const char* value);

        // skips values and whitespace after each
        void skip(token_stream& stream, const cpp_cursor& cur,
                  std::initializer_list<const char*> values);

        bool skip_if_token(detail::token_stream& stream, const char* token);

        template <typename Func>
        void skip_bracket_count(detail::token_stream& stream, const cpp_cursor& cur,
                                const char* open, const char* close, Func f)
        {
            detail::skip_whitespace(stream);
            detail::skip(stream, cur, open);

            auto bracket_count = 1;
            auto expr_bracket  = 0;
            while (bracket_count != 0 || expr_bracket != 0)
            {
                auto spelling = stream.get().get_value();
                if (expr_bracket == 0 && spelling == open)
                    ++bracket_count;
                else if (expr_bracket == 0 && spelling == close)
                    --bracket_count;
                else if (expr_bracket == 0 && std::strcmp(close, ">") == 0 && spelling == ">>")
                    bracket_count -= 2;
                else if (spelling == "(")
                    ++expr_bracket;
                else if (spelling == ")")
                    --expr_bracket;

                f(spelling.c_str());
            }
        }

        inline void skip_bracket_count(detail::token_stream& stream, const cpp_cursor& cur,
                                       const char* open, const char* close)
        {
            skip_bracket_count(stream, cur, open, close, [](const char*) {});
        }

        // skips an attribute if any
        void skip_attribute(detail::token_stream& stream, const cpp_cursor& cur);

        class tokenizer
        {
        public:
            static std::string read_source(cpp_cursor cur);

            static CXFile read_range(cpp_cursor cur, unsigned& begin_offset, unsigned& end_offset);

            tokenizer(translation_unit& tu, cpp_cursor cur);

            context::iterator_type begin(unsigned offset = 0)
            {
                assert(offset < source_.size());
                return impl_->begin(source_.begin() + offset, source_.end());
            }

            context::iterator_type end()
            {
                return impl_->end();
            }

            const std::string& get_source() const STANDARDESE_NOEXCEPT
            {
                return source_;
            }

        private:
            std::string source_;
            context*    impl_;
        };

        inline token_stream make_stream(tokenizer& t, unsigned offset = 0)
        {
            auto last =
                boost::wave::cpplexer::lex_token<>(boost::wave::token_id::T_SEMICOLON, ";", {});
            return token_stream(t.begin(offset), t.end(), last);
        }
    }
} // namespace standardese::detail

#endif // STANDARDESE_DETAIL_TOKENIZER_HPP_INCLUDED
