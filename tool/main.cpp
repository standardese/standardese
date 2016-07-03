// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <cmark_version.h>
#include <spdlog/spdlog.h>

#include <standardese/error.hpp>
#include <standardese/generator.hpp>
#include <standardese/parser.hpp>
#include <standardese/output.hpp>

#include "filesystem.hpp"
#include "options.hpp"
#include "thread_pool.hpp"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

constexpr auto terminal_width = 100u; // assume 100 columns for terminal help text

void print_version(const char* exe_name)
{
    std::clog << exe_name << " version " << STANDARDESE_VERSION_MAJOR << '.'
              << STANDARDESE_VERSION_MINOR << '\n';
    std::clog << "Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>\n";
    std::clog << '\n';
    std::clog << "Using libclang version: " << standardese::string(clang_getClangVersion()).c_str()
              << '\n';
    std::clog << "Using cmark version: " << CMARK_VERSION_STRING << '\n';
}

void print_usage(const char* exe_name, const po::options_description& generic,
                 const po::options_description& configuration)
{
    std::clog << "Usage: " << exe_name << " [options] inputs\n";
    std::clog << '\n';
    std::clog << generic << '\n';
    std::clog << '\n';
    std::clog << configuration << '\n';
}

int main(int argc, char* argv[])
{
    // clang-format off
    po::options_description generic("Generic options", terminal_width), configuration("Configuration", terminal_width);
    generic.add_options()
            ("version,V", "prints version information and exits")
            ("help,h", "prints this help message and exits")
            ("config,c", po::value<fs::path>(), "read options from additional config file as well")
            ("verbose,v", po::value<bool>()->implicit_value(true)->default_value(false),
             "prints more information")
            ("jobs,j", po::value<unsigned>()->default_value(standardese_tool::default_no_threads()),
             "sets the number of threads to use")
            ("color", po::value<bool>()->implicit_value(true)->default_value(true),
             "enable/disable color support of logger");

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
            ("input.blacklist_entity_name",
             po::value<std::vector<std::string>>()->default_value({}, "(none)"),
             "C++ entity names (and all children) that are forbidden")
            ("input.blacklist_namespace",
             po::value<std::vector<std::string>>()->default_value({}, "(none)"),
             "C++ namespace names (with all children) that are forbidden")
            ("input.force_blacklist",
             po::value<bool>()->implicit_value(true)->default_value(false),
             "force the blacklist for explictly given files")
            ("input.require_comment",
             po::value<bool>()->implicit_value(true)->default_value(true),
             "only generates documentation for entities that have a documentation comment")
            ("input.extract_private",
             po::value<bool>()->implicit_value(true)->default_value(false),
             "whether or not to document private entities")

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
             "override name for the command following the name_ (e.g. comment.cmd_name_requires=require)")
            ("comment.implicit_paragraph", po::value<bool>()->implicit_value(true)->default_value(false),
             "whether or not each line in the documentation comment is one paragraph")

            ("output.section_name_", po::value<std::string>(),
             "override output name for the section following the name_ (e.g. output.section_name_requires=Require)")
            ("output.tab_width", po::value<unsigned>()->default_value(4),
             "the tab width (i.e. number of spaces, won't emit tab) of the code in the synthesis");
    // clang-format on

    standardese_tool::configuration config;
    try
    {
        config = standardese_tool::get_configuration(argc, argv, generic, configuration);

        if (config.map.at("jobs").as<unsigned>() == 0)
            throw std::invalid_argument("number of threads must not be 0");
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        print_usage(argv[0], generic, configuration);
        return 1;
    }

    auto& map            = config.map;
    auto& parser         = *config.parser;
    auto& compile_config = config.compile_config;
    auto& log            = parser.get_logger();

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
    else
        try
        {
            using namespace standardese;

            parser.get_comment_config().set_command_character(
                map.at("comment.command_character").as<char>());
            parser.get_comment_config().set_implicit_paragraph(
                map.at("comment.implicit_paragraph").as<bool>());

            parser.get_output_config().set_tab_width(map.at("output.tab_width").as<unsigned>());

            auto input           = map.at("input-files").as<std::vector<fs::path>>();
            auto blacklist_ext   = map.at("input.blacklist_ext").as<std::vector<std::string>>();
            auto blacklist_file  = map.at("input.blacklist_file").as<std::vector<std::string>>();
            auto blacklist_dir   = map.at("input.blacklist_dir").as<std::vector<std::string>>();
            auto force_blacklist = map.at("input.force_blacklist").as<bool>();

            auto& blacklist_entity = parser.get_output_config().get_blacklist();
            for (auto& str : map.at("input.blacklist_entity_name").as<std::vector<std::string>>())
                blacklist_entity.blacklist(str);
            for (auto& str : map.at("input.blacklist_namespace").as<std::vector<std::string>>())
                blacklist_entity.blacklist(str);
            if (map.at("input.require_comment").as<bool>())
                blacklist_entity.set_option(entity_blacklist::require_comment);
            if (map.at("input.extract_private").as<bool>())
                blacklist_entity.set_option(entity_blacklist::extract_private);

            log->debug("Using libclang version: {}", string(clang_getClangVersion()).c_str());
            log->debug("Using cmark version: {}", CMARK_VERSION_STRING);

            standardese_tool::thread_pool  pool(map.at("jobs").as<unsigned>());
            std::vector<std::future<void>> futures;
            futures.reserve(input.size());

            assert(!input.empty());
            for (auto& path : input)
            {
                auto generate = [&](const fs::path& p) {
                    log->info() << "Generating documentation for " << p << "...";

                    try
                    {
                        auto tu = parser.parse(p.generic_string().c_str(), compile_config);

                        auto doc = generate_doc_file(parser, tu.get_file());

                        output_format_xml format;
                        file_output file(p.stem().generic_string() + '.' + format.extension());
                        output      out(file, format);
                        out.render(*doc);
                    }
                    catch (libclang_error& ex)
                    {
                        log->error("libclang error on {}", ex.what());
                    }
                };

                auto handle = [&](const fs::path& p) {
                    futures.push_back(standardese_tool::add_job(pool, generate, p));
                };

                auto res = standardese_tool::handle_path(path, blacklist_ext, blacklist_file,
                                                         blacklist_dir, handle);
                if (!res && !force_blacklist)
                    // path is a normal file that is on the blacklist
                    // blacklist isn't enforced however
                    handle(path);
            }

            for (auto& f : futures)
                f.get();
        }
        catch (std::exception& ex)
        {
            log->critical(ex.what());
            return 1;
        }
}
