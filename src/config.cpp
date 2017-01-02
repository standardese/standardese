// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <standardese/config.hpp>

#include <clang-c/CXCompilationDatabase.h>
#include <set>
#include <spdlog/fmt/fmt.h>

#include <standardese/detail/tokenizer.hpp>
#include <standardese/detail/wrapper.hpp>
#include <standardese/error.hpp>
#include <standardese/section.hpp>

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

    const char* flags[int(compile_flag::count)] = {};

    void init_flags()
    {
        flags[int(compile_flag::ms_extensions)]    = "-fms-extensions";
        flags[int(compile_flag::ms_compatibility)] = "-fms-compatibility";
    }

    auto flags_initializer = (init_flags(), 0);

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

    std::set<std::string> get_db_args(const char* commands_dir)
    {
        auto error   = CXCompilationDatabase_NoError;
        auto db_impl = clang_CompilationDatabase_fromDirectory(commands_dir, &error);
        if (error != CXCompilationDatabase_NoError)
            throw libclang_error(error == CXCompilationDatabase_CanNotLoadDatabase ?
                                     CXError_InvalidArguments :
                                     CXError_Failure,
                                 std::string("CXCompilationDatabase (") + commands_dir + ")");

        database              db(db_impl);
        std::set<std::string> db_args;

        commands cmds(clang_CompilationDatabase_getAllCompileCommands(db.get()));
        auto     num = clang_CompileCommands_getSize(cmds.get());
        for (auto i = 0u; i != num; ++i)
        {
            auto cmd     = clang_CompileCommands_getCommand(cmds.get(), i);
            auto no_args = clang_CompileCommand_getNumArgs(cmd);

            auto        was_ignored = false;
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

        return db_args;
    }

    // cmake sucks at string handling, so sometimes LIBCLANG_SYSTEM_INCLUDE_DIR isn't a string
    // so we need to stringify it
    // but if the argument was a string, libclang can't handle the double quotes
    // so unstringify it then at runtime (you can't do that in the preprocessor...)
    std::string unquote(const char* str)
    {
        if (str[0] == '"')
        {
            std::string res(str + 1);
            res.pop_back();
            return res;
        }

        return str;
    }

#define STANDARDESE_DETAIL_STRINGIFY_IMPL(x) #x
#define STANDARDESE_DETAIL_STRINGIFY(x) STANDARDESE_DETAIL_STRINGIFY_IMPL(x)

    std::string get_clang_binary_default()
    {
        return unquote(STANDARDESE_DETAIL_STRINGIFY(CLANG_BINARY));
    }
}

compile_config::compile_config(cpp_standard standard, string commands_dir)
: flags_{"-x", "c++", "-I", unquote(STANDARDESE_DETAIL_STRINGIFY(LIBCLANG_SYSTEM_INCLUDE_DIR))},
  clang_binary_(get_clang_binary_default())
{
    (void)standards_initializer;

    if (!commands_dir.empty())
    {
        auto db_args = get_db_args(commands_dir.c_str());

        flags_.reserve(flags_.size() + db_args.size());
        for (auto& arg : db_args)
            flags_.push_back(arg);
    }

    if (standard != cpp_standard::count)
        flags_.push_back(standards[int(standard)]);
}

void compile_config::add_macro_definition(string def)
{
    flags_.push_back("-D");
    flags_.push_back(std::move(def));
}

void compile_config::remove_macro_definition(string def)
{
    flags_.push_back("-U");
    flags_.push_back(std::move(def));
}

void compile_config::add_include(string path)
{
    flags_.push_back("-I");
    flags_.push_back(std::move(path));
}

void compile_config::set_flag(compile_flag f)
{
    auto str = flags[int(f)];
    assert(str);
    flags_.push_back(str);
}

void compile_config::set_msvc_compatibility_version(unsigned version)
{
    if (version != 0u)
        flags_.push_back(fmt::format("-fms-compatibility-version={}", version));
}

std::vector<const char*> compile_config::get_flags() const
{
    std::vector<const char*> result;
    result.reserve(flags_.size());

    for (auto& flag : flags_)
        result.push_back(flag.c_str());

    return result;
}

comment_config::comment_config() : cmd_char_('\\')
{
#define STANDARDESE_DETAIL_SET(type) set_command(unsigned(section_type::type), #type);

    STANDARDESE_DETAIL_SET(brief)
    STANDARDESE_DETAIL_SET(details)

    STANDARDESE_DETAIL_SET(requires)
    STANDARDESE_DETAIL_SET(effects)
    STANDARDESE_DETAIL_SET(synchronization)
    STANDARDESE_DETAIL_SET(postconditions)
    STANDARDESE_DETAIL_SET(returns)
    STANDARDESE_DETAIL_SET(throws)
    STANDARDESE_DETAIL_SET(complexity)
    STANDARDESE_DETAIL_SET(remarks)
    STANDARDESE_DETAIL_SET(error_conditions)
    STANDARDESE_DETAIL_SET(notes)

    STANDARDESE_DETAIL_SET(see)

#undef STANDARDESE_DETAIL_SET
#define STANDARDESE_DETAIL_SET(type) set_command(unsigned(command_type::type), #type);

    STANDARDESE_DETAIL_SET(exclude)
    STANDARDESE_DETAIL_SET(unique_name)
    STANDARDESE_DETAIL_SET(synopsis)
    STANDARDESE_DETAIL_SET(synopsis_return)

    STANDARDESE_DETAIL_SET(group)
    STANDARDESE_DETAIL_SET(module)
    STANDARDESE_DETAIL_SET(output_section)

    STANDARDESE_DETAIL_SET(entity)
    STANDARDESE_DETAIL_SET(file)
    STANDARDESE_DETAIL_SET(param)
    STANDARDESE_DETAIL_SET(tparam)
    STANDARDESE_DETAIL_SET(base)

#undef STANDARDESE_DETAIL_SET
}

unsigned comment_config::try_get_command(const std::string& command) const STANDARDESE_NOEXCEPT
{
    auto iter = commands_.find(command);
    return iter == commands_.end() ? unsigned(command_type::invalid) : iter->second;
}

unsigned comment_config::get_command(const std::string& command) const
{
    auto iter = commands_.find(command);
    if (iter == commands_.end())
        throw std::invalid_argument(fmt::format("Invalid command name '{}'", command));
    return iter->second;
}

void comment_config::set_command(unsigned t, std::string command)
{
    // erase old command name
    for (auto iter = commands_.begin(); iter != commands_.end(); ++iter)
        if (iter->second == t)
        {
            commands_.erase(iter);
            break;
        }

    // insert new name
    auto res = commands_.emplace(std::move(command), unsigned(t));
    if (!res.second)
        throw std::invalid_argument(fmt::format("Command name '{}' already in use", command));
}

output_config::output_config()
: section_names_(std::size_t(section_type::count)),
  hidden_name_("'hidden'"),
  tab_width_(4u),
  flags_(0u)
{
#define STANDARDESE_DETAIL_SET(type, name) section_names_[unsigned(section_type::type)] = name;

    STANDARDESE_DETAIL_SET(brief, "")
    STANDARDESE_DETAIL_SET(details, "")

    STANDARDESE_DETAIL_SET(requires, "Requires")
    STANDARDESE_DETAIL_SET(effects, "Effects")
    STANDARDESE_DETAIL_SET(synchronization, "Synchronization")
    STANDARDESE_DETAIL_SET(postconditions, "Postconditions")
    STANDARDESE_DETAIL_SET(returns, "Returns")
    STANDARDESE_DETAIL_SET(throws, "Throws")
    STANDARDESE_DETAIL_SET(complexity, "Complexity")
    STANDARDESE_DETAIL_SET(remarks, "Remarks")
    STANDARDESE_DETAIL_SET(error_conditions, "Error conditions")
    STANDARDESE_DETAIL_SET(notes, "Notes")

    STANDARDESE_DETAIL_SET(see, "See also")

#undef STANDARDESE_DETAIL_SET
}

void output_config::set_section_name(section_type t, std::string name)
{
    if (t == section_type::brief || t == section_type::details)
        throw std::logic_error("Cannot override section name for brief or details");
    section_names_[unsigned(t)] = std::move(name);
}
