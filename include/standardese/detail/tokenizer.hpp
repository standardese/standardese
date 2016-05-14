// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_TOKENIZER_HPP_INCLUDED
#define STANDARDESE_DETAIL_TOKENIZER_HPP_INCLUDED

#include <initializer_list>
#include <string>

#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include <boost/wave.hpp>

#include <standardese/detail/sequence_stream.hpp>
#include <standardese/string.hpp>

namespace standardese
{
    class cpp_cursor;
    class translation_unit;
} // namespace standardese

namespace standardese { namespace detail
{
    using token_iterator = boost::wave::cpplexer::lex_iterator<boost::wave::cpplexer::lex_token<>>;
    using context = boost::wave::context<std::string::const_iterator, detail::token_iterator>;
    using token_stream = sequence_stream<context::iterator_type>;

    // skips all whitespace
    void skip_whitespace(token_stream &stream);

    // skips "value" and asserts that it is actually there
    void skip(token_stream &stream, const char *value);

    // skips values and whitespace after each
    void skip(token_stream &stream, std::initializer_list<const char *> values);

    bool skip_if_token(detail::token_stream &stream, const char *token);

    template <typename Func>
    void skip_bracket_count(detail::token_stream &stream,
                            const char *open, const char *close,
                            Func f)
    {
        detail::skip_whitespace(stream);
        detail::skip(stream, open);

        auto bracket_count = 1;
        while (bracket_count != 0)
        {
            auto spelling = stream.get().get_value();
            if (spelling == open)
                ++bracket_count;
            else if (spelling == close)
                --bracket_count;

            f(spelling.c_str());
        }
    }

    inline void skip_bracket_count(detail::token_stream &stream,
                            const char *open, const char *close)
    {
        skip_bracket_count(stream, open, close, [](const char *) {});
    }

    class tokenizer
    {
    public:
        tokenizer(translation_unit &tu, cpp_cursor cur);

        context::iterator_type begin()
        {
            return impl_->begin(source_.begin(), source_.end());
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
        context *impl_;
    };

    inline token_stream make_stream(tokenizer &t)
    {
        auto last = boost::wave::cpplexer::lex_token<>(boost::wave::token_id::T_SEMICOLON, ";", {});
        return token_stream(t, last);
    }
}} // namespace standardese::detail

#endif // STANDARDESE_DETAIL_TOKENIZER_HPP_INCLUDED
