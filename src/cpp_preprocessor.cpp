// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/cpp_preprocessor.hpp>

#include <boost/config.hpp>
#include <boost/filesystem.hpp>
#include <boost/version.hpp>

#if (BOOST_VERSION / 100000) != 1
#error "require Boost 1.x"
#endif

#if ((BOOST_VERSION / 100) % 1000) < 55
#warning "Boost less than 1.55 isn't tested"
#endif

#include <standardese/detail/tokenizer.hpp>
#include <standardese/config.hpp>
#include <standardese/error.hpp>
#include <standardese/parser.hpp>
#include <standardese/translation_unit.hpp>

// treat the tiny-process-library as header only
#include <process.hpp>
#include <process.cpp>
#ifdef BOOST_WINDOWS
#include <process_win.cpp>
#else
#include <process_unix.cpp>
#endif

using namespace standardese;

namespace fs = boost::filesystem;

namespace
{
    const char* guard_suffixes[] = {"_HPP_INCLUDED", "_H_INCLUDED"};

    // dumb heuristic
    bool is_guard(const std::string& name)
    {
        for (auto suffix : guard_suffixes)
        {
            auto length = std::strlen(suffix);
            if (name.size() <= length)
                continue;
            else if (std::strncmp(name.c_str() + name.size() - length, suffix, length) == 0)
                return true;
        }
        return false;
    }
}

cpp_ptr<standardese::cpp_macro_definition> cpp_macro_definition::parse(CXTranslationUnit tu,
                                                                       CXFile file, cpp_cursor cur,
                                                                       const cpp_entity& parent,
                                                                       unsigned          line_no)
{
    detail::tokenizer tokenizer(tu, file, cur);

    std::string name = tokenizer.begin()[0].get_value().c_str();
    if (is_guard(name))
        return nullptr;

    auto function_like = false;
#if CINDEX_VERSION_MINOR >= 33
    function_like = clang_Cursor_isMacroFunctionLike(cur) != 0u;
#else
    // first token after name decides if it is function like
    auto token = tokenizer.begin()[1];
    if (token.get_value() == "(")
    {
        // it is an open parenthesis
        // but could be part of the macro replacement
        // so check if whitespace in between
        auto name_token = *tokenizer.begin();
        function_like =
            (name_token.get_offset() + name_token.get_value().length() == token.get_offset());
    }
#endif

    std::string params, replacement;
    if (function_like)
    {
        params    = "(";
        auto iter = tokenizer.begin() + 2; // 0 is name, 1 is open parenthesis
        for (; iter->get_value() != ")"; ++iter)
            detail::append_token(params, iter->get_value());
        params += ")";

        for (++iter; iter != tokenizer.end(); ++iter)
            detail::append_token(replacement, iter->get_value());
    }
    else
    {
        for (auto iter = tokenizer.begin() + 1; iter != tokenizer.end(); ++iter)
            detail::append_token(replacement, iter->get_value());
    }

    return detail::make_cpp_ptr<cpp_macro_definition>(parent, std::move(name), std::move(params),
                                                      std::move(replacement), line_no);
}

namespace
{
    std::string get_command(const compile_config& c, const char* full_path)
    {
        // -E: print preprocessor output
        // -C: keep comments
        // -Wno-pragma-once-outside-header: hide wrong warning
        std::string cmd(fs::path(c.get_clang_binary()).generic_string()
                        + " -E -CC -Wno-pragma-once-outside-header ");
        for (auto& flag : c)
        {
            cmd += '"' + std::string(flag.c_str()) + '"';
            cmd += ' ';
        }

        cmd += full_path;
        return cmd;
    }

    std::string get_full_preprocess_output(const parser& p, const compile_config& c,
                                           const char* full_path)
    {
        std::string preprocessed;

        auto    cmd = get_command(c, full_path);
        Process process(cmd, "",
                        [&](const char* str, std::size_t n) {
                            preprocessed.reserve(preprocessed.size() + n);
                            for (auto end = str + n; str != end; ++str)
                                if (*str != '\r')
                                    preprocessed.push_back(*str);
                        },
                        [&](const char* str, std::size_t n) {
                            p.get_logger()->error("[preprocessor] {}", std::string(str, n));
                        });

        auto exit_code = process.get_exit_status();
        if (exit_code != 0)
            throw process_error(cmd, exit_code);

        return preprocessed;
    }

    struct line_marker
    {
        enum flag_t
        {
            enter_new = 1, // flag 1
            enter_old = 2, // flag 2
            system    = 4, // flag 3
        };

        std::string file_name;
        unsigned    line, flags;

        line_marker() : line(0u), flags(0u)
        {
        }

        void set_flag(flag_t f)
        {
            flags |= f;
        }

        bool is_set(flag_t f) const
        {
            return (flags & f) != 0;
        }

        bool none_set() const
        {
            return flags == 0u;
        }
    };

    // preprocessor line marker
    // format: # <line> "<file-name>" <flags>
    // flag 1 - start of a new file
    // flag 2 - returning to previous file
    // flag 3 - system header
    // flag 4 is irrelevant
    line_marker parse_line_marker(const char*& ptr)
    {
        line_marker result;

        assert(*ptr == '#');
        ++ptr;

        while (*ptr == ' ')
            ++ptr;

        std::string line;
        while (std::isdigit(*ptr))
            line += *ptr++;
        result.line = unsigned(std::stoi(line));

        while (*ptr == ' ')
            ++ptr;

        assert(*ptr == '"');
        ++ptr;

        std::string file_name;
        while (*ptr != '"')
            file_name += *ptr++;
        ++ptr;
        result.file_name = std::move(file_name);

        while (*ptr == ' ')
            ++ptr;

        while (*ptr != '\n')
        {
            if (*ptr == ' ')
                ++ptr;

            switch (*ptr)
            {
            case '1':
                result.set_flag(line_marker::enter_new);
                break;
            case '2':
                result.set_flag(line_marker::enter_old);
                break;
            case '3':
                result.set_flag(line_marker::system);
                break;
            case '4':
                break;
            default:
                assert(false);
            }
            ++ptr;
        }

        return result;
    }

    CXTranslationUnit get_cxunit(CXIndex index, const compile_config& c, const char* full_path)
    {
        auto args = c.get_flags();

        CXTranslationUnit tu;
        auto              error =
            clang_parseTranslationUnit2(index, full_path, args.data(),
                                        static_cast<int>(args.size()), nullptr, 0,
                                        CXTranslationUnit_Incomplete
                                            | CXTranslationUnit_DetailedPreprocessingRecord,
                                        &tu);
        if (error != CXError_Success)
            throw libclang_error(error, "CXTranslationUnit (" + std::string(full_path) + ")");
        return tu;
    }

    void add_macros(const parser& p, const compile_config& c, const char* full_path, cpp_file& file,
                    const std::vector<unsigned>& fake_lines)
    {
        detail::tu_wrapper tu(get_cxunit(p.get_cxindex(), c, full_path));
        auto               cxfile = clang_getFile(tu.get(), full_path);
        auto               iter   = fake_lines.begin();

        detail::visit_tu(tu.get(), full_path, [&](cpp_cursor cur, cpp_cursor) {
            if (clang_getCursorKind(cur) == CXCursor_MacroDefinition)
            {
                auto     loc = clang_getCursorLocation(cur);
                unsigned line;
                clang_getSpellingLocation(loc, nullptr, &line, nullptr, nullptr);

                iter = std::lower_bound(iter, fake_lines.end(), line);
                file.add_entity(cpp_macro_definition::parse(tu.get(), cxfile, cur, file,
                                                            (iter - fake_lines.begin()) + line));
            }
            return CXChildVisit_Continue;
        });
    }
}

std::string preprocessor::preprocess(const parser& p, const compile_config& c,
                                     const char* full_path, cpp_file& file) const
{
    std::string           preprocessed;
    std::vector<unsigned> fake_lines;

    auto full_preprocessed = get_full_preprocess_output(p, c, full_path);
    auto line_no           = 1u;
    auto file_depth        = 0;
    auto was_newl = true, in_c_comment = false, write_char = true;
    for (auto ptr = full_preprocessed.c_str(); *ptr; ++ptr)
    {
        if (*ptr == '\n')
        {
            was_newl = true;
        }
        else if (in_c_comment && ptr[0] == '*' && ptr[1] == '/')
        {
            in_c_comment = false;
            was_newl     = false;
            // add an additional newline
            // this allows using c style doc comments in macros
            // normally macros would all be one line, so each entity gets the same comment
            if (file_depth == 0)
            {
                preprocessed += '\n';
                fake_lines.push_back(line_no);
            }
        }
        else if (*ptr == '/' && ptr[1] == '*')
        {
            in_c_comment = true;
            was_newl     = false;
        }
        else if (was_newl && !in_c_comment && *ptr == '#' && ptr[1] != 'p')
        {
            auto marker = parse_line_marker(ptr);
            assert(*ptr == '\n');

            if (marker.file_name == full_path)
            {
                assert(file_depth <= 1);
                file_depth = 0;
                write_char = false;

                if (marker.none_set())
                {
                    if (line_no < marker.line)
                    {
                        auto diff = marker.line - line_no;
                        preprocessed.append(diff, '\n');
                    }
                    line_no = marker.line;
                }
            }
            else if (marker.is_set(line_marker::enter_new))
            {
                ++file_depth;
                if (file_depth == 1 && marker.file_name != "<built-in>"
                    && marker.file_name != "<command line>")
                {
                    // write include
                    preprocessed += "#include ";
                    if (marker.is_set(line_marker::system))
                        preprocessed += '<';
                    else
                        preprocessed += '"';
                    preprocessed += marker.file_name;
                    if (marker.is_set(line_marker::system))
                        preprocessed += '>';
                    else
                        preprocessed += '"';
                    preprocessed += '\n';
                    ++line_no;

                    // also add include
                    if (is_whitelisted_directory(marker.file_name))
                        file.add_entity(
                            cpp_inclusion_directive::make(file, marker.file_name,
                                                          marker.is_set(line_marker::system) ?
                                                              cpp_inclusion_directive::system :
                                                              cpp_inclusion_directive::local,
                                                          marker.line));
                }
            }
            else if (marker.is_set(line_marker::enter_old))
            {
                --file_depth;
            }
        }
        else
            was_newl = false;

        if (file_depth == 0 && write_char)
        {
            preprocessed += *ptr;
            if (*ptr == '\n')
                ++line_no;
        }
        else if (file_depth == 0)
            write_char = true;
    }

    add_macros(p, c, full_path, file, fake_lines);
    return preprocessed;
}

void preprocessor::whitelist_include_dir(std::string dir)
{
    auto path = fs::system_complete(dir).normalize().generic_string();
    if (!path.empty() && path.back() == '.')
        path.pop_back();
    include_dirs_.insert(std::move(path));
}

bool preprocessor::is_whitelisted_directory(std::string& dir) const STANDARDESE_NOEXCEPT
{
    auto path = fs::system_complete(dir).normalize();

    std::string result;
    for (auto iter = path.begin(); iter != path.end(); ++iter)
    {
        result += iter->generic_string();
        if (result.back() != '/')
            result += '/';

        if (include_dirs_.count(result))
        {
            dir.clear();
            for (++iter; iter != path.end(); ++iter)
            {
                if (!dir.empty())
                    dir += '/';
                dir += iter->generic_string();
            }
            return true;
        }
    }
    return false;
}
