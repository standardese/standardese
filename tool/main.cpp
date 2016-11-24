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
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <standardese/error.hpp>
#include <standardese/generator.hpp>
#include <standardese/index.hpp>
#include <standardese/output.hpp>
#include <standardese/parser.hpp>
#include <standardese/template_processor.hpp>

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

template <typename Generator>
std::vector<standardese::documentation> generate_documentation(
    standardese::parser& parser, const po::variables_map& map, standardese_tool::thread_pool& pool,
    std::vector<standardese::template_file>& templates, Generator generate)
{
    auto input              = map.at("input-files").as<std::vector<fs::path>>();
    auto source_ext         = map.at("input.source_ext").as<std::vector<std::string>>();
    auto blacklist_ext      = map.at("input.blacklist_ext").as<std::vector<std::string>>();
    auto blacklist_file     = map.at("input.blacklist_file").as<std::vector<std::string>>();
    auto blacklist_dir      = map.at("input.blacklist_dir").as<std::vector<std::string>>();
    auto blacklist_dotfiles = map.at("input.blacklist_dotfiles").as<bool>();
    auto force_blacklist    = map.at("input.force_blacklist").as<bool>();

    assert(!input.empty());
    for (auto& path : input)
        parser.get_preprocessor().whitelist_include_dir(path.parent_path().generic_string());

    std::vector<std::future<standardese::documentation>> futures;
    futures.reserve(input.size());

    for (auto& path : input)
        standardese_tool::
            handle_path(path, source_ext, blacklist_ext, blacklist_file, blacklist_dir,
                        blacklist_dotfiles, force_blacklist,
                        [&](bool is_source_file, const fs::path& p, const fs::path& relative) {
                            if (is_source_file)
                                futures.push_back(
                                    standardese_tool::add_job(pool, generate, p, relative));
                            else
                            {
                                std::ifstream file(p.generic_string());
                                if (!file.is_open())
                                    parser.get_logger()->error("unable to open template file '{}",
                                                               p.generic_string());
                                templates
                                    .emplace_back(standardese_tool::get_output_name(relative)
                                                      + relative.extension().generic_string(),
                                                  std::string(std::istreambuf_iterator<char>(file),
                                                              std::istreambuf_iterator<char>{}));
                            }
                        });

    std::vector<standardese::documentation> documentations;
    for (auto& f : futures)
    {
        auto doc = f.get();
        if (doc.document)
            documentations.push_back(std::move(doc));
    }

    return documentations;
}

void write_output_files(const standardese_tool::configuration& config,
                        const standardese::index& idx, standardese_tool::thread_pool& pool,
                        const std::vector<standardese::documentation>& documentations,
                        const std::vector<standardese::raw_document>&  raw_documents)
{
    using namespace standardese;

    for (auto& format : config.formats)
    {
        config.parser->get_logger()->info("Writing files for output format {}...",
                                          format->extension());

        path prefix;
        if (config.formats.size() > 1u)
        {
            fs::create_directories(format->extension());
            prefix = format->extension();
            prefix += '/'; // hope that every platform handles it
        }

        output out(idx, prefix, *format);
        standardese_tool::for_each(pool, documentations,
                                   [](const standardese::documentation& doc) {
                                       return doc.document != nullptr;
                                   },
                                   [&](const standardese::documentation& doc) {
                                       out.render(config.parser->get_logger(), *doc.document,
                                                  config.link_extension());
                                   });
        standardese_tool::for_each(pool, raw_documents,
                                   [](const standardese::raw_document&) { return true; },
                                   [&](const standardese::raw_document& doc) {
                                       out.render_raw(config.parser->get_logger(), doc);
                                   });
    }
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
            ("input.source_ext",
             po::value<std::vector<std::string>>()
             ->default_value({".h", ".hpp", ".h++", ".hxx"}, "(common C++ header file extensions"),
            "file extensions that are treated as header files and where files will be parsed")
            ("input.blacklist_ext",
             po::value<std::vector<std::string>>()->default_value({}, "(none)"),
             "file extension that is forbidden (e.g. \".md\"; \".\" for no extension)")
            ("input.blacklist_file",
             po::value<std::vector<std::string>>()->default_value({}, "(none)"),
             "file that is forbidden, relative to traversed directory")
            ("input.blacklist_dir",
             po::value<std::vector<std::string>>()->default_value({}, "(none)"),
             "directory that is forbidden, relative to traversed directory")
            ("input.blacklist_dotfiles",
             po::value<bool>()->implicit_value(true)->default_value(true),
             "whether or not dotfiles are blacklisted")
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
            ("compilation.preprocess_dir,P", po::value<std::vector<std::string>>(),
             "whitelists all includes to that directory so that they show up in the output")
            ("compilation.ms_extensions",
             po::value<bool>()->implicit_value(true)->default_value(standardese_tool::default_msvc_comp()),
             "enable/disable MSVC extension support (-fms-extensions)")
            ("compilation.ms_compatibility",
             po::value<unsigned>()->default_value(standardese_tool::default_msvc_version()),
             "set MSVC compatibility version to fake, 0 to disable (-fms-compatibility[-version])")
            ("compilation.clang_binary", po::value<std::string>(),
             "path to clang++ binary")

            ("comment.command_character", po::value<char>()->default_value('\\'),
             "character used to introduce special commands")
            ("comment.cmd_name_", po::value<std::string>(),
             "override name for the command following the name_ (e.g. comment.cmd_name_requires=require)")
            ("comment.external_doc", po::value<std::vector<std::string>>()->default_value({}, ""),
             "syntax is prefix=url, supports linking to a different URL for entities starting with prefix")

            ("template.delimiter_begin", po::value<std::string>()->default_value("{{"),
             "set the template delimiter begin string")
            ("template.delimiter_end", po::value<std::string>()->default_value("}}"),
            "set the template delimiter end string")
            ("template.cmd_name_", po::value<std::string>(),
            "override the name for the template command following the name_ (e.g. template.cmd_name_if=my_if);"
            "standardese prefix will be added automatically")

            ("output.format",
             po::value<std::vector<std::string>>()->default_value(std::vector<std::string>{"commonmark"}, "{commonmark}"),
             "the output format used (commonmark, latex, man, html, xml)")
            ("output.link_extension", po::value<std::string>(),
             "the file extension of the links to entities, useful if you convert standardese output to a different format and change the extension")
            ("output.section_name_", po::value<std::string>(),
             "override output name for the section following the name_ (e.g. output.section_name_requires=Require)")
            ("output.tab_width", po::value<unsigned>()->default_value(4),
             "the tab width (i.e. number of spaces, won't emit tab) of the code in the synthesis")
            ("output.width", po::value<unsigned>()->default_value(terminal_width),
             "the width of the output (used in e.g. commonmark format)")
            ("output.inline_doc", po::value<bool>()->default_value(true)->implicit_value(true),
             "whether or not some entity documentation (parameters etc.) will be shown inline")
            ("output.show_macro_replacement", po::value<bool>()->default_value(false)->implicit_value(true),
             "whether or not the replacement of macros will be shown")
            ("output.show_group_member_id", po::value<bool>()->default_value(true)->implicit_value(true),
             "whether or not to show the index of member group members in the synopsis")
            ("output.show_modules", po::value<bool>()->default_value(true)->implicit_value(true),
            "whether or not the module of an entity is shown in the documentation");
    // clang-format on

    standardese_tool::configuration config;
    try
    {
        config = standardese_tool::get_configuration(argc, argv, generic, configuration);
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
            log->debug("Using libclang version: {}", string(clang_getClangVersion()).c_str());
            log->debug("Using cmark version: {}", CMARK_VERSION_STRING);

            standardese_tool::thread_pool pool(map.at("jobs").as<unsigned>());
            standardese::index            index;
            config.set_external(index.get_linker());

            // generate documentations
            auto generate = [&](const fs::path& p, const fs::path& relative) {
                log->info("Generating documentation for {}...", p);

                standardese::documentation result(nullptr);
                try
                {
                    auto output_name = standardese_tool::get_output_name(relative);

                    auto tu = parser.parse(p.generic_string().c_str(), compile_config,
                                           relative.generic_string().c_str());
                    result = generate_doc_file(parser, index, tu.get_file(), output_name);
                }
                catch (libclang_error& ex)
                {
                    log->error("libclang error on {}", ex.what());
                }
                catch (cmark_error& ex)
                {
                    log->error("cmark error in '{}'", ex.what());
                }

                return result;
            };

            std::vector<template_file> templates;
            auto documentations = generate_documentation(parser, map, pool, templates, generate);

            // generate indices
            log->info("Generating indices...");
            documentations.push_back(documentation(generate_file_index(index)));
            documentations.push_back(documentation(generate_entity_index(index)));
            documentations.push_back(documentation(generate_module_index(parser, index)));

            // process templates
            auto raw_documents =
                standardese_tool::for_each(pool, templates,
                                           [](const template_file&) { return true; },
                                           [&](const template_file& f) {
                                               return process_template(parser, index, f);
                                           });

            // write output
            write_output_files(config, index, pool, documentations, raw_documents);
        }
        catch (std::exception& ex)
        {
            log->critical(ex.what());
            return 1;
        }
}
