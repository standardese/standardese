// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_DETAIL_TOKENIZER_HPP_INCLUDED
#define STANDARDESE_DETAIL_TOKENIZER_HPP_INCLUDED

#include <initializer_list>
#include <string>

#include <standardese/detail/sequence_stream.hpp>
#include <standardese/string.hpp>
#include <standardese/cpp_cursor.hpp>
#include <standardese/translation_unit.hpp>

namespace standardese
{
    struct cpp_cursor;
    class translation_unit;
} // namespace standardese

namespace standardese
{
    namespace detail
    {
        CXFile get_range(const translation_unit& tu, cpp_cursor cur, unsigned& begin_offset,
                         unsigned& end_offset);

        CXFile get_range(CXSourceRange extent, unsigned& begin_offset, unsigned& end_offset);

        class token
        {
        public:
            token() : token("", CXToken_Punctuation)
            {
            }

            token(string value, CXTokenKind kind, unsigned offset = 0)
            : value_(value), kind_(kind), offset_(offset)
            {
            }

            token(CXTranslationUnit tu, CXToken token);

            const token* operator->() const STANDARDESE_NOEXCEPT
            {
                return this;
            }

            string get_value() const STANDARDESE_NOEXCEPT
            {
                return value_;
            }

            CXTokenKind get_kind() const STANDARDESE_NOEXCEPT
            {
                return kind_;
            }

            unsigned get_offset() const STANDARDESE_NOEXCEPT
            {
                return offset_;
            }

        private:
            string      value_;
            CXTokenKind kind_;
            unsigned    offset_;
        };

        class token_iterator : public std::iterator<std::bidirectional_iterator_tag, token>
        {
        public:
            token operator*() const STANDARDESE_NOEXCEPT
            {
                return token(tu_, *cx_token_);
            }

            token operator->() const STANDARDESE_NOEXCEPT
            {
                return token(tu_, *cx_token_);
            }

            token_iterator& operator++() STANDARDESE_NOEXCEPT
            {
                ++cx_token_;
                return *this;
            }

            token_iterator operator++(int)STANDARDESE_NOEXCEPT
            {
                token_iterator tmp(*this);
                ++*this;
                return tmp;
            }

            token_iterator& operator--() STANDARDESE_NOEXCEPT
            {
                --cx_token_;
                return *this;
            }

            token_iterator operator--(int)STANDARDESE_NOEXCEPT
            {
                token_iterator tmp(*this);
                --*this;
                return tmp;
            }

            friend bool operator==(const token_iterator& a,
                                   const token_iterator& b) STANDARDESE_NOEXCEPT
            {
                return a.cx_token_ == b.cx_token_;
            }

            friend bool operator!=(const token_iterator& a,
                                   const token_iterator& b) STANDARDESE_NOEXCEPT
            {
                return !(a == b);
            }

        private:
            token_iterator(CXTranslationUnit tu, CXToken* token) STANDARDESE_NOEXCEPT
                : tu_(tu),
                  cx_token_(token)
            {
            }

            CXTranslationUnit tu_;
            CXToken*          cx_token_;

            friend class tokenizer;
        };

        class tokenizer
        {
        public:
            tokenizer(CXTranslationUnit tu, CXFile file, cpp_cursor cur);

            tokenizer(const translation_unit& tu, cpp_cursor cur)
            : tokenizer(tu.get_cxunit(), tu.get_cxfile(), cur)
            {
            }

            ~tokenizer() STANDARDESE_NOEXCEPT;

            tokenizer(tokenizer&&) = delete;
            tokenizer& operator=(tokenizer&&) = delete;

            token_iterator begin() const STANDARDESE_NOEXCEPT
            {
                return token_iterator(get_cxunit(), tokens_);
            }

            token_iterator end() const STANDARDESE_NOEXCEPT;

            // returns whether two '>' characters at the end were munched into a single '>>'
            // only necessary for template parameter
            bool need_unmunch() const STANDARDESE_NOEXCEPT;

            CXTranslationUnit get_cxunit() const STANDARDESE_NOEXCEPT
            {
                return tu_;
            }

            CXFile get_cxfile() const STANDARDESE_NOEXCEPT
            {
                return file_;
            }

            token end_token() const STANDARDESE_NOEXCEPT
            {
                return token(end_, CXToken_Punctuation);
            }

        private:
            CXTranslationUnit tu_;
            CXFile            file_;
            CXToken*          tokens_;
            unsigned          no_tokens_, end_offset_;
            const char*       end_;
        };

        using token_stream = sequence_stream<token_iterator>;

        inline token_stream make_stream(const tokenizer& t)
        {
            return token_stream(t.begin(), t.end(), t.end_token());
        }

        // skips until behind offset
        void skip_offset(token_stream& stream, unsigned offset);

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
        bool skip_attribute(detail::token_stream& stream, const cpp_cursor& cur);
    }
} // namespace standardese::detail

#endif // STANDARDESE_DETAIL_TOKENIZER_HPP_INCLUDED
