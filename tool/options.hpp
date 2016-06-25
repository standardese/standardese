// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_OPTIONS_HPP_INCLUDED
#define STANDARDESE_OPTIONS_HPP_INCLUDED

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <spdlog/spdlog.h>

#include <standardese/config.hpp>
#include <standardese/parser.hpp>

namespace standardese_tool
{
    namespace detail
    {
        inline standardese::cpp_standard parse_standard(const std::string& str)
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
    } // namespace detail

    inline standardese::compile_config parse_config(
        const boost::program_options::variables_map& map)
    {
        using namespace standardese;

        auto standard = detail::parse_standard(map.at("compilation.standard").as<std::string>());
        auto dir      = map.find("compilation.commands_dir");

        compile_config result(standard, dir == map.end() ? "" : dir->second.as<std::string>());

        auto incs = map.find("compilation.include_dir");
        if (incs != map.end())
            for (auto& val : incs->second.as<std::vector<std::string>>())
                result.add_include(val);

        auto defs = map.find("compilation.macro_definition");
        if (defs != map.end())
            for (auto& val : defs->second.as<std::vector<std::string>>())
                result.add_macro_definition(val);

        auto undefs = map.find("compilation.macro_undefinition");
        if (undefs != map.end())
            for (auto& val : undefs->second.as<std::vector<std::string>>())
                result.remove_macro_definition(val);

        return result;
    }

    namespace detail
    {
        inline bool erase_prefix(std::string& str, const std::string& prefix)
        {
            auto res = str.find(prefix);
            if (res != 0u)
                return false;
            str.erase(0, prefix.size());
            return true;
        }

        inline void handle_unparsed_options(standardese::parser&                          p,
                                            const boost::program_options::parsed_options& options)
        {
            using namespace standardese;

            for (auto& opt : options.options)
                if (opt.unregistered)
                {
                    auto name = opt.string_key;

                    if (erase_prefix(name, "comment.cmd_name_"))
                    {
                        auto section = p.get_comment_config().get_section(name);
                        p.get_comment_config().set_section_command(section, opt.value[0]);
                    }
                    else if (erase_prefix(name, "output.section_name_"))
                    {
                        auto section = p.get_comment_config().get_section(name);
                        p.get_output_config().set_section_name(section, opt.value[0]);
                    }
                    else
                        throw std::invalid_argument("unrecognized option '" + opt.string_key + "'");
                }
        }
    } // namespace detail

    inline std::unique_ptr<standardese::parser> get_parser(
        const boost::program_options::variables_map&  map,
        const boost::program_options::parsed_options& cmd_result,
        const boost::program_options::parsed_options& file_result)
    {
        auto log = spdlog::stdout_logger_mt("standardese_log", map.at("color").as<bool>());
        log->set_pattern("[%l] %v");
        if (map.at("verbose").as<bool>())
            log->set_level(spdlog::level::debug);

        auto p = std::unique_ptr<standardese::parser>(new standardese::parser(log));
        detail::handle_unparsed_options(*p, cmd_result);
        detail::handle_unparsed_options(*p, file_result);

        return p;
    }

    struct configuration
    {
        std::unique_ptr<standardese::parser>  parser;
        standardese::compile_config           compile_config;
        boost::program_options::variables_map map;

        configuration() : compile_config(standardese::cpp_standard::cpp_14)
        {
        }

        configuration(std::unique_ptr<standardese::parser> p, standardese::compile_config c,
                      boost::program_options::variables_map map)
        : parser(std::move(p)), compile_config(std::move(c)), map(std::move(map))
        {
        }
    };

    inline configuration get_configuration(
        int argc, char* argv[], const boost::program_options::options_description& generic,
        const boost::program_options::options_description& configuration)
    {
        namespace po = boost::program_options;
        namespace fs = boost::filesystem;

        po::variables_map map;

        po::options_description input("");
        input.add_options()("input-files", po::value<std::vector<fs::path>>(), "input files");
        po::positional_options_description input_pos;
        input_pos.add("input-files", -1);

        po::options_description cmd;
        cmd.add(generic).add(configuration).add(input);

        auto cmd_result = po::command_line_parser(argc, argv)
                              .options(cmd)
                              .positional(input_pos)
                              .allow_unregistered()
                              .run();
        po::store(cmd_result, map);
        po::notify(map);

        auto               iter = map.find("config");
        po::parsed_options file_result(nullptr);
        if (iter != map.end())
        {
            auto          path = iter->second.as<fs::path>();
            std::ifstream config(path.string());
            if (!config.is_open())
                throw std::runtime_error("config file '" + path.generic_string() + "' not found");

            po::options_description conf;
            conf.add(configuration);

            file_result = po::parse_config_file(config, configuration, true);
            po::store(file_result, map);
            po::notify(map);
        }

        auto config = parse_config(map);
        auto parser = get_parser(map, cmd_result, file_result);

        return {std::move(parser), std::move(config), std::move(map)};
    }
} // namespace standardese_tool

#endif // STANDARDESE_OPTIONS_HPP_INCLUDED
