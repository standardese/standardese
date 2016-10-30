// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_OPTIONS_HPP_INCLUDED
#define STANDARDESE_OPTIONS_HPP_INCLUDED

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <spdlog/spdlog.h>

#include <standardese/config.hpp>
#include <standardese/index.hpp>
#include <standardese/output_format.hpp>
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

    inline bool default_msvc_comp() noexcept
    {
#ifdef _MSC_VER
        return true;
#else
        return false;
#endif
    }

    inline unsigned default_msvc_version() noexcept
    {
#ifdef _MSC_VER
        return _MSC_VER / 100u;
#else
        return 0u;
#endif
    }

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

        if (map.at("compilation.ms_extensions").as<bool>())
            result.set_flag(compile_flag::ms_extensions);

        if (auto version = map.at("compilation.ms_compatibility").as<unsigned>())
        {
            result.set_flag(compile_flag::ms_compatibility);
            result.set_msvc_compatibility_version(version);
        }

        auto binary = map.find("compilation.clang_binary");
        if (binary != map.end())
            result.set_clang_binary(binary->second.as<std::string>());

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
                        auto section = p.get_comment_config().get_command(name);
                        p.get_comment_config().set_command(section, opt.value[0]);
                    }
                    else if (erase_prefix(name, "output.section_name_"))
                    {
                        auto section = p.get_comment_config().get_command(name);
                        if (!standardese::is_section(section))
                            throw std::invalid_argument(name + " is not a section");
                        p.get_output_config().set_section_name(standardese::make_section(section),
                                                               opt.value[0]);
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

        auto dirs = map.find("compilation.preprocess_dir");
        if (dirs != map.end())
            for (auto& dir : dirs->second.as<std::vector<std::string>>())
                p->get_preprocessor().whitelist_include_dir(std::move(dir));

        return p;
    }

    struct configuration
    {
        std::vector<std::unique_ptr<standardese::output_format_base>> formats;
        std::unique_ptr<standardese::parser>                          parser;
        standardese::compile_config                                   compile_config;
        boost::program_options::variables_map                         map;

        configuration() : compile_config(standardese::cpp_standard::cpp_14)
        {
        }

        configuration(std::unique_ptr<standardese::parser> p, standardese::compile_config c,
                      boost::program_options::variables_map map)
        : parser(std::move(p)), compile_config(std::move(c)), map(std::move(map))
        {
            auto width = this->map.at("output.width").as<unsigned>();
            for (auto& format_str : this->map.at("output.format").as<std::vector<std::string>>())
            {
                if (format_str == "commonmark")
                    formats.emplace_back(new standardese::output_format_markdown(width));
                else if (format_str == "latex")
                    formats.emplace_back(new standardese::output_format_latex(width));
                else if (format_str == "man")
                    formats.emplace_back(new standardese::output_format_man(width));
                else if (format_str == "html")
                    formats.emplace_back(new standardese::output_format_html);
                else if (format_str == "xml")
                    formats.emplace_back(new standardese::output_format_xml);
                else
                    throw std::invalid_argument("unknown format '" + format_str + "'");
            }
        }

        const char* link_extension() const
        {
            auto iter = map.find("output.link_extension");
            if (iter != map.end())
                return iter->second.as<std::string>().c_str();
            return nullptr;
        }

        void set_external(standardese::index& i) const
        {
            // register cppreference.com
            i.register_external("std::", "http://en.cppreference.com/mwiki/"
                                         "index.php?title=Special%3ASearch&search=$$");
            for (auto& str : map.at("comment.external_doc").as<std::vector<std::string>>())
            {
                auto sep    = str.find('=');
                auto prefix = str.substr(0, sep);
                auto url    = str.substr(sep + 1);
                i.register_external(std::move(prefix), std::move(url));
            }
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
