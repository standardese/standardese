// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

void print_version(const char *exe_name)
{
    std::clog << exe_name << " version " << STANDARDESE_VERSION_MAJOR << '.' << STANDARDESE_VERSION_MINOR << '\n';
    std::clog << "Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>\n";
}

void print_usage(const char *exe_name, const po::options_description &generic)
{
    std::clog << "Usage: " << exe_name << " [options] inputs\n";
    std::clog << '\n';
    std::clog << generic << '\n';
}

int main(int argc, char** argv)
{
    po::options_description generic("Generic options");
    generic.add_options()
            ("version,v", "prints version information and exits")
            ("help,h", "prints this help message and exits");

    po::options_description input("");
    input.add_options()
            ("input-files", po::value<std::vector<std::string>>(), "input files");
    po::positional_options_description input_pos;
    input_pos.add("input-files", -1);

    po::options_description cmd;
    cmd.add(generic).add(input);

    po::variables_map map;
    try
    {
        po::store(po::command_line_parser(argc, argv).options(cmd).positional(input_pos).run(), map);
        po::notify(map);
    }
    catch (boost::program_options::error &ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';
        print_usage(argv[0], generic);
        return 1;
    }

    if (map.count("help"))
        print_usage(argv[0], generic);
    else if (map.count("version"))
        print_version(argv[0]);
    else if (map.count("input-files") == 0u)
    {
        std::cerr << "Error: no input file specified\n";
        std::cerr << '\n';
        print_usage(argv[0], generic);
        return 1;
    }
    else
    {
        auto input = map["input-files"].as<std::vector<std::string>>();
        assert(!input.empty());

        std::cout << "Input files:\n";
        for (auto& str : input)
            std::cout << '\t' << str << '\n';
        std::cout << '\n';
    }
}
