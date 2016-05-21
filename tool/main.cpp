// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <spdlog/spdlog.h>

#include <standardese/comment.hpp>
#include <standardese/error.hpp>
#include <standardese/generator.hpp>
#include <standardese/parser.hpp>

#include "filesystem.hpp"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

void print_version(const char *exe_name)
{
    std::clog << exe_name << " version " << STANDARDESE_VERSION_MAJOR << '.' << STANDARDESE_VERSION_MINOR << '\n';
    std::clog << "Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>\n";
}

void print_usage(const char *exe_name,
                 const po::options_description &generic,
                 const po::options_description &configuration)
{
    std::clog << "Usage: " << exe_name << " [options] inputs\n";
    std::clog << '\n';
    std::clog << generic << '\n';
    std::clog << '\n';
    std::clog << configuration << '\n';
}

bool erase_prefix(std::string &str, const std::string &prefix)
{
    auto res = str.find(prefix);
    if (res != 0u)
        return false;
    str.erase(0, prefix.size());
    return true;
}

void handle_unparsed_options(const po::parsed_options &options)
{
    using namespace standardese;

    for (auto& opt : options.options)
        if (opt.unregistered)
        {
            auto name = opt.string_key;

            if (erase_prefix(name, "comment.cmd_name_"))
            {
                comment::parser::set_section_command(name, opt.value[0]);
            }
            else if (erase_prefix(name, "output.section_name_"))
            {
                comment::parser::set_section_name(name, opt.value[0]);
            }
            else
               throw std::invalid_argument("unrecognized option '" + opt.string_key + "'");
        }
}

standardese::cpp_standard parse_standard(const std::string &str)
{
    using namespace standardese;

    if (str == "c++98")
        return cpp_standard::cpp_98;
    else if (str == "c++03")
        return cpp_standard::cpp_03;
    else if (str == "c++11")
        return cpp_standard::cpp_11;
    else if (str == "c++14")
        return cpp_standard::cpp_14;
    else
        throw std::invalid_argument("invalid C++ standard '" + str + "'");
}

standardese::compile_config parse_config(const po::variables_map &map)
{
    using namespace standardese;
    compile_config result(parse_standard(map.at("compilation.standard").as<std::string>()));

    auto dir = map.find("compilation.commands_dir");
    if (dir != map.end())
        result.commands_dir = dir->second.as<std::string>();

    auto incs = map.find("compilation.include_dir");
    if (incs != map.end())
        for (auto& val : incs->second.as<std::vector<std::string>>())
            result.options.push_back(compile_config::include_directory(val));

    auto defs = map.find("compilation.macro_definition");
    if (defs != map.end())
        for (auto& val : defs->second.as<std::vector<std::string>>())
            result.options.push_back(compile_config::macro_definition(val));

    auto undefs = map.find("compilation.macro_undefinition");
    if (undefs != map.end())
        for (auto& val : undefs->second.as<std::vector<std::string>>())
            result.options.push_back(compile_config::macro_undefinition(val));

    return result;
}

int main(int argc, char** argv)
{
    auto log = spdlog::stdout_logger_mt("standardese_log", true);
    log->set_pattern("[%l] %v");

    po::options_description generic("Generic options"), configuration("Configuration");
    generic.add_options()
            ("version,v", "prints version information and exits")
            ("help,h", "prints this help message and exits")
            ("config,c", po::value<fs::path>(), "read options from additional config file as well");

    configuration.add_options()
            ("input.blacklist_ext",
             po::value<std::vector<std::string>>()->default_value({}, "(none)"),
             "file extension that is forbidden (e.g. \".md\"; \".\" for no extension)")
            ("input.blacklist_file",
             po::value<std::vector<std::string>>()->default_value({}, "(none)"),
             "file that is forbidden, relative to traversed directory")
            ("input.blacklist_dir",
             po::value<std::vector<std::string>>()->default_value({}, "(none)"),
             "directory that is forbidden, relative to traversed directory")
            ("input.force_blacklist", "force the blacklist for explictly given files")

            ("compilation.commands_dir", po::value<std::string>(),
             "the directory where a compile_commands.json is located, its options have lower priority than the other ones")
            ("compilation.standard", po::value<std::string>()->default_value("c++14"),
             "the C++ standard to use for parsing, valid values are c++98/03/11/14")
            ("compilation.include_dir,I", po::value<std::vector<std::string>>(),
             "adds an additional include directory to use for parsing")
            ("compilation.macro_definition,D", po::value<std::vector<std::string>>(),
             "adds an implicit #define before parsing")
            ("compilation.macro_undefinition,U", po::value<std::vector<std::string>>(),
             "adds an implicit #undef before parsing")

            ("comment.command_character", po::value<char>()->default_value('\\'),
             "character used to introduce special commands")
            ("comment.cmd_name_", po::value<std::string>(),
             "override name for the command following the name_ (e.g. comment.cmd_name_requires=Require)")

            ("output.section_name_", po::value<std::string>(),
             "override output name for the section following the name_ (e.g. output.section_name_requires=Require,"
             "note: override for command name is also required here)");


    po::options_description input("");
    input.add_options()
            ("input-files", po::value<std::vector<fs::path>>(), "input files");
    po::positional_options_description input_pos;
    input_pos.add("input-files", -1);

    po::options_description cmd;
    cmd.add(generic).add(configuration).add(input);

    po::variables_map map;
    standardese::compile_config config("");
    try
    {
        auto cmd_result = po::command_line_parser(argc, argv).options(cmd)
                             .positional(input_pos).allow_unregistered().run();
        po::parsed_options file_result(nullptr);

        po::store(cmd_result, map);
        po::notify(map);

        auto iter = map.find("config");
        if (iter != map.end())
        {
            auto path = iter->second.as<fs::path>();
            std::ifstream config(path.string());
            if (!config.is_open())
                throw std::runtime_error("config file '" + path.generic_string() + "' not found");

            po::options_description conf;
            conf.add(configuration);
            file_result = po::parse_config_file(config, configuration, true);
            po::store(file_result, map);
            po::notify(map);
        }

        handle_unparsed_options(cmd_result);
        handle_unparsed_options(file_result);

        config = parse_config(map);
    }
    catch (std::exception &ex)
    {
        log->critical(ex.what());
        print_usage(argv[0], generic, configuration);
        return 1;
    }

    if (map.count("help"))
        print_usage(argv[0], generic, configuration);
    else if (map.count("version"))
        print_version(argv[0]);
    else if (map.count("input-files") == 0u)
    {
        log->critical("no input file(s) specified");
        print_usage(argv[0], generic, configuration);
        return 1;
    }
    else try
    {
        using namespace standardese;

        comment::parser::set_command_character(map["comment.command_character"].as<char>());

        auto input = map["input-files"].as<std::vector<fs::path>>();
        auto blacklist_ext = map["input.blacklist_ext"].as<std::vector<std::string>>();
        auto blacklist_file = map["input.blacklist_file"].as<std::vector<std::string>>();
        auto blacklist_dir = map["input.blacklist_dir"].as<std::vector<std::string>>();
        auto force_blacklist = map.count("input.force_blacklist") != 0u;

        assert(!input.empty());

        for (auto& path : input)
        {
            parser parser(log);

            auto handle = [&](const fs::path &p)
            {
                log->info() << "Generating documentation for " << p << "...";

                try
                {
                    auto tu = parser.parse(p.generic_string().c_str(), config);
                    auto& f = tu.build_ast();

                    file_output file(p.stem().generic_string() + ".md");
                    markdown_output out(file);
                    generate_doc_file(parser, out, f);
                }
                catch (libclang_error &ex)
                {
                    log->error("libclang error on {}", ex.what());
                }
            };

            auto res = standardese_tool::handle_path(path, blacklist_ext, blacklist_file, blacklist_dir, handle);
            if (!res && !force_blacklist)
                // path is a normal file that is on the blacklist
                // blacklist isn't enforced however
                handle(path);
        }
    }
    catch (std::exception &ex)
    {
        log->critical(ex.what());
        return 1;
    }
}
