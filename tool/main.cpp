// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

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

int main(int argc, char** argv)
{
    po::options_description generic("Generic options"), configuration("Configuration");
    generic.add_options()
            ("version,v", "prints version information and exits")
            ("help,h", "prints this help message and exits")
            ("config,c", po::value<fs::path>(), "read options from additional config file as well");
    configuration.add_options()
            ("input.blacklist_ext",
             po::value<std::vector<std::string>>()->default_value({}, "(none)"),
             "file extension that is forbidden (e.g. \".md\"; \".\" for no extension)");

    po::options_description input("");
    input.add_options()
            ("input-files", po::value<std::vector<fs::path>>(), "input files");
    po::positional_options_description input_pos;
    input_pos.add("input-files", -1);

    po::options_description cmd;
    cmd.add(generic).add(configuration).add(input);

    po::variables_map map;
    try
    {
        po::store(po::command_line_parser(argc, argv).options(cmd).positional(input_pos).run(), map);
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
            po::store(po::parse_config_file(config, configuration), map);
            po::notify(map);
        }
    }
    catch (std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';
        print_usage(argv[0], generic, configuration);
        return 1;
    }

    if (map.count("help"))
        print_usage(argv[0], generic, configuration);
    else if (map.count("version"))
        print_version(argv[0]);
    else if (map.count("input-files") == 0u)
    {
        std::cerr << "Error: no input file specified\n";
        std::cerr << '\n';
        print_usage(argv[0], generic, configuration);
        return 1;
    }
    else try
    {
        auto input = map["input-files"].as<std::vector<fs::path>>();
        auto blacklist_ext = map["input.blacklist_ext"].as<std::vector<std::string>>();

        assert(!input.empty());

        std::cout << "Input files:\n";
        for (auto& path : input)
        {
            if (fs::is_regular_file(path))
                std::cout << '\t' << path.generic_string() << '\n';
            else if (fs::is_directory(path))
            {
                std::for_each(fs::recursive_directory_iterator(path), fs::recursive_directory_iterator(),
                            [&](const fs::directory_entry &entry)
                            {
                                if (entry.status().type() != fs::file_type::regular_file)
                                    return;

                                auto entry_ext = entry.path().extension();
                                for (auto& ext : blacklist_ext)
                                    if (ext == entry_ext || (entry_ext.empty() && ext == "."))
                                        return;

                                std::cout << '\t' << entry.path() << '\n';
                            });
            }
            else if (fs::exists(path))
                throw std::runtime_error("file '" + path.generic_string() + "' is neither file nor directory");
            else
                throw std::runtime_error("file '" + path.generic_string() + "' does not exist");
        }
        std::cout << '\n';
    }
    catch (std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
