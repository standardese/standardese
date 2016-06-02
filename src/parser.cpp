// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/parser.hpp>

#include <clang-c/CXCompilationDatabase.h>
#include <set>

#include <spdlog/sinks/null_sink.h>

#include <standardese/error.hpp>
#include <standardese/string.hpp>
#include <standardese/translation_unit.hpp>

using namespace standardese;

namespace
{
    struct database_deleter
    {
        void operator()(CXCompilationDatabase db) const STANDARDESE_NOEXCEPT
        {
            clang_CompilationDatabase_dispose(db);
        }
    };

    using database = detail::wrapper<CXCompilationDatabase, database_deleter>;

    struct commands_deleter
    {
        void operator()(CXCompileCommands db) const STANDARDESE_NOEXCEPT
        {
            clang_CompileCommands_dispose(db);
        }
    };

    using commands = detail::wrapper<CXCompileCommands, commands_deleter>;
}

#define STANDARDESE_DETAIL_STRINGIFY_IMPL(x) #x
#define STANDARDESE_DETAIL_STRINGIFY(x) STANDARDESE_DETAIL_STRINGIFY_IMPL(x)

translation_unit parser::parse(const char *path, const compile_config &c) const
{
    // cmake sucks at string handling, so sometimes LIBCLANG_SYSTEM_INCLUDE_DIR isn't a string
    // so we need to stringify it
    // but if the argument was a string, libclang can't handle the double qoutes
    // so unstringify it then at runtime (you can't do that in the preprocessor...)
    const char* basic_args[] = {"-x", "c++", "-I", STANDARDESE_DETAIL_STRINGIFY(LIBCLANG_SYSTEM_INCLUDE_DIR)};

    std::vector<const char*> args(basic_args, basic_args + sizeof(basic_args) / sizeof(const char*));
    std::string include_dir;
    if (args.back()[0] == '"')
    {
        // unstringify
        include_dir = args.back() + 1;
        include_dir.pop_back();
        args.back() = include_dir.c_str();
    }

    std::set<std::string> db_args; // need std::string to own the arguments
    if (!c.commands_dir.empty())
    {
        auto error = CXCompilationDatabase_NoError;
        database db(clang_CompilationDatabase_fromDirectory(c.commands_dir.c_str(), &error));
        if (error != CXCompilationDatabase_NoError)
            throw libclang_error(error == CXCompilationDatabase_CanNotLoadDatabase ? CXError_InvalidArguments
                                                                                   : CXError_Failure,
                                 "CXCompilationDatabase (" + c.commands_dir + ")");

        commands cmds(clang_CompilationDatabase_getAllCompileCommands(db.get()));
        auto num = clang_CompileCommands_getSize(cmds.get());
        for (auto i = 0u; i != num; ++i)
        {
            auto cmd = clang_CompileCommands_getCommand(cmds.get(), i);
            auto no_args = clang_CompileCommand_getNumArgs(cmd);

            auto was_ignored = false;
            std::string cur;
            for (auto j = 1u; j != no_args; ++j)
            {
                string str(clang_CompileCommand_getArg(cmd, j));

                // skip -c arg and -o arg
                if (str == "-c" || str == "-o")
                    was_ignored = true;
                else if (was_ignored)
                    was_ignored = false;
                else if (*str.c_str() == '-')
                {
                    // we have an option
                    // store it to later append with parameter
                    // but if there is no option, it will be non-empty on the next option
                    if (!cur.empty())
                    {
                        db_args.insert(std::move(cur));
                        cur.clear();
                    }
                    cur += str.c_str();
                }
                else
                {
                    db_args.insert(cur + str.c_str());
                    cur.clear();
                }
            }
        }

        args.reserve(args.size() + db_args.size());
        for (auto &arg : db_args)
            args.push_back(arg.c_str());
    }

    if (c.standard != cpp_standard::count)
        args.push_back(detail::to_option(c.standard));

    args.reserve(args.size() + 2 * c.options.size());
    for (auto& o : c.options)
        args.push_back(o.c_str());

    CXTranslationUnit tu;
    auto error = clang_parseTranslationUnit2(index_.get(), path, args.data(), args.size(), nullptr, 0,
                                         CXTranslationUnit_Incomplete | CXTranslationUnit_DetailedPreprocessingRecord,
                                         &tu);
    if (error != CXError_Success)
        throw libclang_error(error, "CXTranslationUnit (" + std::string(path) + ")");

    cpp_ptr<cpp_file> file(new cpp_file(clang_getTranslationUnitCursor(tu), path));
    auto file_ptr = file.get();
    files_.add_file(std::move(file));

    return translation_unit(*this, tu, path, file_ptr);
}

parser::parser()
: parser(std::make_shared<spdlog::logger>("null", std::make_shared<spdlog::sinks::null_sink_mt>()))
{}

parser::parser(std::shared_ptr<spdlog::logger> logger)
: index_(clang_createIndex(1, 1)), logger_(std::move(logger))
{}

parser::~parser() STANDARDESE_NOEXCEPT {}

void parser::deleter::operator()(CXIndex idx) const STANDARDESE_NOEXCEPT
{
    clang_disposeIndex(idx);
}