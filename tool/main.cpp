// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <iostream>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

void print_version(const char *exe_name)
{
    std::clog << exe_name << " version " << STANDARDESE_VERSION_MAJOR << '.' << STANDARDESE_VERSION_MINOR << '\n';
    std::clog << "Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>\n";
}

void print_usage(const char *exe_name, const po::options_description &generic)
{
    print_version(exe_name);
    std::clog << '\n';
    std::clog << generic << '\n';
}

int main(int argc, char** argv)
{

    po::options_description generic("Generic options");
    generic.add_options()
            ("version,v", "prints version information and exits")
            ("help,h", "prints this help message and exits");

    po::options_description cmd;
    cmd.add(generic);

    po::variables_map map;
    po::store(po::command_line_parser(argc, argv).options(cmd).run(), map);
    po::notify(map);

    if (map.count("help"))
        print_usage(argv[0], generic);
    else if (map.count("version"))
        print_version(argv[0]);
}

