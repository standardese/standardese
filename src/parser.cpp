// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/parser.hpp>

#include <clang-c/CXCompilationDatabase.h>
#include <mutex>
#include <set>

#include <spdlog/sinks/null_sink.h>

#include <standardese/cpp_namespace.hpp>
#include <standardese/cpp_type.hpp>
#include <standardese/error.hpp>
#include <standardese/translation_unit.hpp>
#include <standardese/string.hpp>

using namespace standardese;

namespace
{
    const char* standards[int(cpp_standard::count)];

    void init_standards()
    {
        standards[int(cpp_standard::cpp_98)] = "-std=c++98";
        standards[int(cpp_standard::cpp_03)] = "-std=c++03";
        standards[int(cpp_standard::cpp_11)] = "-std=c++11";
        standards[int(cpp_standard::cpp_14)] = "-std=c++14";
    }

    auto standards_initializer = (init_standards(), 0);
}

std::string compile_config::include_directory(std::string s)
{
    return "-I" + std::move(s);
}

std::string compile_config::macro_definition(std::string s)
{
    return "-D" + std::move(s);
}

std::string compile_config::macro_undefinition(std::string s)
{
    return "-U" + std::move(s);
}

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

translation_unit parser::parse(const char *path, const compile_config &c) const
{
    const char* basic_args[] = {"-x", "c++", "-I", LIBCLANG_SYSTEM_INCLUDE_DIR};
    std::vector<const char*> args(basic_args, basic_args + sizeof(basic_args) / sizeof(const char*));

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
            for (auto j = 1u; j != no_args; ++j)
            {
                string str(clang_CompileCommand_getArg(cmd, j));

                // skip -c arg and -o arg
                if (str == "-c" || str == "-o")
                    was_ignored = true;
                else if (was_ignored)
                    was_ignored = false;
                else
                    db_args.insert(str.get());
            }
        }


        args.reserve(args.size() + db_args.size());
        for (auto &arg : db_args)
            args.push_back(arg.c_str());
    }

    if (c.standard != cpp_standard::count)
        args.push_back(standards[int(c.standard)]);

    args.reserve(args.size() + 2 * c.options.size());
    for (auto& o : c.options)
        args.push_back(o.c_str());

    CXTranslationUnit tu;
    auto error = clang_parseTranslationUnit2(index_.get(), path, args.data(), args.size(), nullptr, 0,
                                         CXTranslationUnit_Incomplete | CXTranslationUnit_DetailedPreprocessingRecord,
                                         &tu);
    if (error != CXError_Success)
        throw libclang_error(error, "CXTranslationUnit (" + std::string(path) + ")");

    return translation_unit(*this, tu, path);
}

namespace
{
    struct type_compare
    {
        bool operator()(cpp_type *a, cpp_type *b) const
        {
            return a->get_unique_name() < b->get_unique_name();
        }
    };
}

struct parser::impl
{
    std::mutex file_mutex;
    std::vector<cpp_ptr<cpp_file>> files;

    std::mutex ns_mutex;
    std::vector<cpp_namespace*> namespaces;
    std::set<cpp_name> namespace_names;

    std::mutex type_mutex;
    std::set<cpp_type*, type_compare> types;
};

parser::parser()
: parser(std::make_shared<spdlog::logger>("null", std::make_shared<spdlog::sinks::null_sink_mt>()))
{}

parser::parser(std::shared_ptr<spdlog::logger> logger)
: index_(clang_createIndex(1, 1)), logger_(std::move(logger)), pimpl_(new impl)
{}

parser::~parser() STANDARDESE_NOEXCEPT {}

void parser::register_file(cpp_ptr<cpp_file> file) const
{
    std::unique_lock<std::mutex> lock(pimpl_->file_mutex);
    pimpl_->files.push_back(std::move(file));
}

void parser::for_each_file(file_callback cb, void* data)
{
    for (auto& ptr : pimpl_->files)
        cb(*ptr, data);
}

void parser::register_namespace(cpp_namespace &n) const
{
    std::unique_lock<std::mutex> lock(pimpl_->ns_mutex);
    pimpl_->namespace_names.insert(n.get_unique_name());
    pimpl_->namespaces.push_back(&n);
}

void parser::for_each_namespace(namespace_callback cb, void *data)
{
    for (auto& n : pimpl_->namespace_names)
        cb(n, data);
}

const cpp_namespace* parser::for_each_in_namespace(const cpp_name &n, in_namespace_callback cb, void *data)
{
    const cpp_namespace* res = nullptr;
    for (auto& ns : pimpl_->namespaces)
    {
        if (ns->get_name() != n)
            continue;
        res = ns;
        for (auto& e : *ns)
            cb(e, data);
    }

    return res;
}

void parser::for_each_in_namespace(in_namespace_callback cb, void *data)
{
    for (auto& f : pimpl_->files)
        for (auto& e : *f)
            cb(e, data);

    for (auto& ns : pimpl_->namespaces)
    {
        for (auto& e : *ns)
            cb(e, data);
    }
}

void parser::register_type(cpp_type &t) const
{
    std::unique_lock<std::mutex> lock(pimpl_->type_mutex);
    pimpl_->types.insert(&t);
}

const cpp_type* parser::lookup_type(const cpp_type_ref &ref) const
{
    struct dummy_type : cpp_type
    {
        dummy_type(cpp_name name)
        : cpp_type(class_t, "", std::move(name), "", {}) {}
    } dummy(ref.get_full_name());

    std::unique_lock<std::mutex> lock(pimpl_->type_mutex);
    auto iter = pimpl_->types.find(&dummy);
    if (iter == pimpl_->types.end())
        return nullptr;
    return *iter;
}

void parser::for_each_type(type_callback cb, void *data)
{
    for (auto& t : pimpl_->types)
        cb(*t, data);
}

void parser::deleter::operator()(CXIndex idx) const STANDARDESE_NOEXCEPT
{
    clang_disposeIndex(idx);
}